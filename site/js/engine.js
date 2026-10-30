/* Motor dos algoritmos.
 *
 * Reproduz o traverse_impl de src/graph_algorithms.tpp: um unico laco que
 * conversa com uma "fronteira" (fila, pilha, vetor ou heap) por meio de
 * push / pop / process / explore. Cada passo vira um snapshot, para a
 * animacao poder andar para frente e para tras.
 */
(function () {
  const INF = Infinity;

  // ---------------------------------------------------------------- grafo
  function buildAdj(graph) {
    const n = graph.nodes.length;
    const adj = Array.from({ length: n + 1 }, () => []);
    for (const e of graph.edges) {
      adj[e.u].push({ u: e.u, v: e.v, w: e.w, id: e.id });
      adj[e.v].push({ u: e.v, v: e.u, w: e.w, id: e.id });
    }
    return adj;
  }

  const entryLess = (a, b) =>
    a.cost !== b.cost ? a.cost < b.cost :
    a.vertex !== b.vertex ? a.vertex < b.vertex :
    a.parent < b.parent;

  // ---------------------------------------------------------- fronteiras
  class Fila {
    constructor() { this.q = []; this.head = 0; this.kind = "fila"; this.work = 0; }
    push(cost, vertex, parent, edge) { this.q.push({ cost, vertex, parent, edge }); }
    pop() { this.work += 1; return this.q[this.head++]; }
    empty() { return this.head >= this.q.length; }
    items() { return this.q.slice(this.head); }
    process(e, par, dist, parEdge) {
      if (e.vertex === e.parent) return true;
      if (par[e.vertex] !== 0) return false;
      par[e.vertex] = e.parent;
      parEdge[e.vertex] = e.edge;
      dist[e.vertex] = dist[e.parent] + 1;
      return true;
    }
    explore(u, ed, par) {
      if (par[ed.v] === 0) { this.push(0, ed.v, u, ed.id); return "pushed"; }
      return "ignored";
    }
  }

  class Pilha extends Fila {
    constructor() { super(); this.s = []; this.kind = "pilha"; }
    push(cost, vertex, parent, edge) { this.s.push({ cost, vertex, parent, edge }); }
    pop() { this.work += 1; return this.s.pop(); }
    empty() { return this.s.length === 0; }
    items() { return this.s.slice(); }
  }

  /* Heap binario com remocao preguicosa, como o priority_queue do C++:
   * cada relaxamento insere uma nova entrada; as velhas sao descartadas
   * no pop quando cost != dist[v]. */
  class Heap {
    constructor() { this.h = []; this.kind = "heap"; this.work = 0; }
    push(cost, vertex, parent, edge) {
      const h = this.h;
      h.push({ cost, vertex, parent, edge });
      let i = h.length - 1;
      while (i > 0) {
        const p = (i - 1) >> 1;
        this.work += 1;
        if (!entryLess(h[i], h[p])) break;
        [h[i], h[p]] = [h[p], h[i]];
        i = p;
      }
    }
    pop() {
      const h = this.h;
      const top = h[0];
      const last = h.pop();
      if (h.length) {
        h[0] = last;
        let i = 0;
        for (;;) {
          const l = 2 * i + 1, r = l + 1;
          let m = i;
          if (l < h.length) { this.work += 1; if (entryLess(h[l], h[m])) m = l; }
          if (r < h.length) { this.work += 1; if (entryLess(h[r], h[m])) m = r; }
          if (m === i) break;
          [h[i], h[m]] = [h[m], h[i]];
          i = m;
        }
      }
      return top;
    }
    empty() { return this.h.length === 0; }
    items() { return this.h.map((e) => ({ ...e })); }
    process(e, par, dist) { return e.cost === dist[e.vertex]; }
    explore(u, ed, par, dist, parEdge) {
      const cand = dist[u] + ed.w;
      if (cand < dist[ed.v]) {
        dist[ed.v] = cand;
        par[ed.v] = u;
        parEdge[ed.v] = ed.id;
        this.push(cand, ed.v, u, ed.id);
        return "relaxed";
      }
      return "ignored";
    }
  }

  /* Vetor: guarda a melhor estimativa de cada vertice pendente e
   * extrai o minimo varrendo todos os n vertices -> O(n) por pop. */
  class Vetor {
    constructor(n) {
      this.n = n;
      this.kind = "vetor";
      this.costs = new Array(n + 1).fill(INF);
      this.parents = new Array(n + 1).fill(0);
      this.edges = new Array(n + 1).fill(null);
      this.pending = new Array(n + 1).fill(false);
      this.count = 0;
      this.work = 0;
      this.lastMin = 0;
    }
    push(cost, vertex, parent, edge) {
      if (!this.pending[vertex]) { this.pending[vertex] = true; this.count++; }
      this.costs[vertex] = cost;
      this.parents[vertex] = parent;
      this.edges[vertex] = edge;
    }
    pop() {
      let best = 0;
      for (let v = 1; v <= this.n; v++) {
        this.work += 1;
        if (!this.pending[v]) continue;
        if (best === 0 || this.costs[v] < this.costs[best]) best = v;
      }
      this.pending[best] = false;
      this.count--;
      this.lastMin = best;
      return { cost: this.costs[best], vertex: best, parent: this.parents[best], edge: this.edges[best] };
    }
    empty() { return this.count === 0; }
    items() {
      const out = [];
      for (let v = 1; v <= this.n; v++) {
        out.push({ vertex: v, cost: this.costs[v], pending: this.pending[v] });
      }
      return out;
    }
    process(e, par, dist) { return e.cost === dist[e.vertex]; }
    explore(u, ed, par, dist, parEdge) {
      const cand = dist[u] + ed.w;
      if (cand < dist[ed.v] && (par[ed.v] === 0 || this.pending[ed.v])) {
        dist[ed.v] = cand;
        par[ed.v] = u;
        parEdge[ed.v] = ed.id;
        this.push(cand, ed.v, u, ed.id);
        return "relaxed";
      }
      return "ignored";
    }
  }

  const ALGS = {
    bfs: { nome: "BFS", fronteira: "Fila", cor: "var(--bfs)", make: () => new Fila(), pesos: false },
    dfs: { nome: "DFS", fronteira: "Pilha", cor: "var(--dfs)", make: () => new Pilha(), pesos: false },
    "dij-heap": { nome: "Dijkstra (heap)", fronteira: "Heap", cor: "var(--dij)", make: () => new Heap(), pesos: true },
    "dij-vetor": { nome: "Dijkstra (vetor)", fronteira: "Vetor", cor: "var(--vec)", make: (n) => new Vetor(n), pesos: true },
  };

  // Linhas do pseudo-codigo exibido (ver CODE_LINES em widgets.js)
  const LINE = { init: 1, pop: 3, discard: 5, visit: 6, explore: 8, done: 9 };

  const fmt = (x) => {
    if (x === INF) return "∞";
    if (Number.isInteger(x)) return String(x);
    return String(Math.round(x * 1000) / 1000);
  };

  // ------------------------------------------------------------- execucao
  function run(graph, start, algKey) {
    const alg = ALGS[algKey];
    const n = graph.nodes.length;
    const adj = buildAdj(graph);

    if (alg.pesos) {
      const neg = graph.edges.filter((e) => e.w < 0);
      if (neg.length) return { error: "negativo", negEdges: neg.map((e) => e.id), steps: [] };
    }
    if (!n || start < 1 || start > n) return { steps: [] };

    const par = new Array(n + 1).fill(0);
    const parEdge = new Array(n + 1).fill(null);
    const dist = new Array(n + 1).fill(INF);
    const order = [];
    const fr = alg.make(n);
    const steps = [];
    let pushes = 0, pops = 0, discards = 0, edgeVisits = 0;

    const snap = (type, extra) => {
      const items = fr.items();
      steps.push({
        type,
        line: LINE[type],
        par: par.slice(),
        parEdge: parEdge.slice(),
        dist: dist.slice(),
        order: order.slice(),
        frontier: items,
        frontierVerts: new Set(
          fr.kind === "vetor" ? items.filter((i) => i.pending).map((i) => i.vertex) : items.map((i) => i.vertex)
        ),
        counters: { pushes, pops, discards, edgeVisits, work: fr.work, size: fr.kind === "vetor" ? fr.count : items.length },
        ...extra,
      });
    };

    par[start] = start;
    dist[start] = 0;
    fr.push(0, start, start, null);
    pushes++;
    snap("init", {
      current: null,
      msg: `<span class="tag">início</span>pai[<b>${start}</b>] = ${start}, dist[<b>${start}</b>] = 0 e a origem entra na ${alg.fronteira.toLowerCase()}.`,
    });

    while (!fr.empty()) {
      const e = fr.pop();
      pops++;
      const popMsg = {
        fila: `retira <b>${e.vertex}</b> da frente da fila`,
        pilha: `retira <b>${e.vertex}</b> do topo da pilha`,
        heap: `extrai o mínimo do heap: <b>${e.vertex}</b> com custo ${fmt(e.cost)}`,
        vetor: `varre os ${n} vértices e acha o mínimo: <b>${e.vertex}</b> com custo ${fmt(e.cost)}`,
      }[fr.kind];
      snap("pop", { current: e.vertex, entry: e, minVertex: fr.kind === "vetor" ? e.vertex : null, msg: `<span class="tag">pop</span>${popMsg}.` });

      if (!fr.process(e, par, dist, parEdge)) {
        discards++;
        const why = fr.kind === "heap"
          ? `entrada obsoleta: custo ${fmt(e.cost)} ≠ dist[${e.vertex}] = ${fmt(dist[e.vertex])}. Descartada (remoção preguiçosa).`
          : `<b>${e.vertex}</b> já tem pai, então já foi visitado. Descartado.`;
        snap("discard", { current: e.vertex, entry: e, msg: `<span class="tag">process</span>${why}` });
        continue;
      }

      order.push(e.vertex);
      const visitMsg = alg.pesos
        ? `<b>${e.vertex}</b> está finalizado: dist = ${fmt(dist[e.vertex])}.`
        : `visita <b>${e.vertex}</b> (pai ${par[e.vertex]}, nível ${dist[e.vertex]}).`;
      snap("visit", { current: e.vertex, msg: `<span class="tag">visita #${order.length}</span>${visitMsg}` });

      for (const ed of adj[e.vertex]) {
        edgeVisits++;
        const before = dist[ed.v];
        const r = fr.explore(e.vertex, ed, par, dist, parEdge);
        if (r === "pushed" || r === "relaxed") pushes++;
        let msg;
        if (r === "pushed") msg = `vizinho <b>${ed.v}</b> ainda sem pai: entra na ${alg.fronteira.toLowerCase()}.`;
        else if (r === "relaxed") msg = `relaxa <b>${e.vertex}→${ed.v}</b>: ${fmt(dist[e.vertex])} + ${fmt(ed.w)} = ${fmt(dist[ed.v])} &lt; ${fmt(before)}. Atualiza pai e dist.`;
        else if (alg.pesos) msg = `<b>${e.vertex}→${ed.v}</b>: ${fmt(dist[e.vertex])} + ${fmt(ed.w)} não melhora ${fmt(dist[ed.v])}.`;
        else msg = `vizinho <b>${ed.v}</b> já tem pai: ignorado.`;
        snap("explore", { current: e.vertex, edge: ed.id, target: ed.v, result: r, changed: r === "ignored" ? null : ed.v, msg: `<span class="tag">explore</span>${msg}` });
      }
    }

    snap("done", {
      current: null,
      msg: `<span class="tag">fim</span>${alg.fronteira} vazia. ${order.length} de ${n} vértices alcançados${order.length < n ? " (os outros estão em outra componente)" : ""}.`,
    });
    return { steps, n, adj };
  }

  // ----------------------------------------------------------- utilidades
  function pathTo(par, v) {
    if (!par[v]) return null;
    const p = [v];
    while (par[v] !== v) { v = par[v]; p.push(v); }
    return p.reverse();
  }

  window.Engine = { run, ALGS, INF, fmt, pathTo, buildAdj };
})();
