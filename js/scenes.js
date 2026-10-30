/* Cenas animadas: capa 3D, construcao da lista, 4 buscas simultaneas,
 * vetor x heap em 3D, arvores por forca, explorador 3D e grafo dos pesquisadores. */
(function () {
  const css = (v) => getComputedStyle(document.documentElement).getPropertyValue(v).trim();
  const { fInt, fNum, fMs, fBytes, fBig } = Charts.fmt;
  const has3D = (() => {
    try {
      const c = document.createElement("canvas");
      return !!(window.ForceGraph3D && (c.getContext("webgl2") || c.getContext("webgl")));
    } catch (e) { return false; }
  })();

  function el(html) {
    const t = document.createElement("template");
    t.innerHTML = html.trim();
    return t.content.firstElementChild;
  }

  function fit3D(G, container) {
    const ro = new ResizeObserver(() => {
      const r = container.getBoundingClientRect();
      if (r.width > 10 && r.height > 10) G.width(r.width).height(r.height);
    });
    ro.observe(container);
  }

  // camera orbitando em volta do centro
  function orbit(G, dist, speed = 0.0025, tilt = 0.25) {
    let a = 0, raf = null, on = true;
    const tick = () => {
      if (on) {
        a += speed;
        G.cameraPosition({ x: dist() * Math.sin(a), y: dist() * tilt, z: dist() * Math.cos(a) }, { x: 0, y: 0, z: 0 });
      }
      raf = requestAnimationFrame(tick);
    };
    return {
      start() { if (!raf) tick(); },
      stop() { cancelAnimationFrame(raf); raf = null; },
      set(v) { on = v; },
      get on() { return on; },
    };
  }

  // =================================================================
  // Capa: grafo 3D girando com "ondas" de BFS
  // =================================================================
  function Cover3D(container) {
    if (!has3D) return Charts.CoverBackground(container);
    const N = 140;
    const cols = [css("--bfs"), css("--dij"), css("--dfs")];
    const nodes = d3.range(N).map((i) => ({ id: i, base: cols[i % 3], hot: 0 }));
    const links = [];
    const seen = new Set();
    for (let i = 1; i < N; i++) {
      const j = Math.floor(Math.random() * i);
      links.push({ source: i, target: j });
      seen.add(`${j}-${i}`);
    }
    for (let k = 0; k < N * 0.4; k++) {
      const a = Math.floor(Math.random() * N), b = Math.floor(Math.random() * N);
      const key = a < b ? `${a}-${b}` : `${b}-${a}`;
      if (a !== b && !seen.has(key)) { seen.add(key); links.push({ source: a, target: b }); }
    }
    const G = ForceGraph3D({ controlType: "orbit" })(container)
      .backgroundColor("rgba(0,0,0,0)")
      .showNavInfo(false)
      .enableNodeDrag(false)
      .enablePointerInteraction(false)
      .nodeRelSize(3.2)
      .nodeResolution(12)
      .nodeOpacity(0.95)
      .nodeVal((n) => 1 + n.hot * 3)
      .nodeColor((n) => (n.hot ? "#ffffff" : n.base))
      .linkColor(() => "#46526c")
      .linkOpacity(0.35)
      .linkWidth(0)
      .cooldownTicks(200)
      .graphData({ nodes, links });
    fit3D(G, container);
    const cam = orbit(G, () => 330, 0.0018, 0.2);
    let wave = null;

    function pulse() {
      const adj = nodes.map(() => []);
      links.forEach((l) => {
        const s = l.source.id ?? l.source, t = l.target.id ?? l.target;
        adj[s].push(t); adj[t].push(s);
      });
      const start = Math.floor(Math.random() * N);
      const lvl = new Map([[start, 0]]);
      const q = [start];
      while (q.length) { const u = q.shift(); for (const v of adj[u]) if (!lvl.has(v)) { lvl.set(v, lvl.get(u) + 1); q.push(v); } }
      lvl.forEach((l, id) => {
        setTimeout(() => { nodes[id].hot = 1; G.nodeColor(G.nodeColor()).nodeVal(G.nodeVal()); }, l * 220);
        setTimeout(() => { nodes[id].hot = 0; G.nodeColor(G.nodeColor()).nodeVal(G.nodeVal()); }, l * 220 + 700);
      });
    }
    return {
      start() { G.resumeAnimation(); cam.start(); pulse(); wave = setInterval(pulse, 5200); },
      stop() { cam.stop(); clearInterval(wave); G.pauseAnimation(); },
    };
  }

  // =================================================================
  // Slide 2: lista de adjacencia sendo construida linha a linha
  // =================================================================
  function LibBuild(root) {
    const base = GraphGen.EXEMPLOS.pdf;
    const linhas = [String(base.nodes.length), ...base.edges.map((e) => `${e.u} ${e.v} ${Engine.fmt(e.w)}`)];
    root.innerHTML = `
      <div class="lib-grid">
        <div class="card">
          <h3>arquivo de entrada</h3>
          <div class="file-lines">${linhas.map((l, i) => `<div class="fl" data-i="${i}"><span class="ln-n">${i + 1}</span>${l}</div>`).join("")}</div>
          <pre class="code lib-code"><span class="ln"><span class="tok-k">void</span> <span class="tok-f">add_edge</span>(<span class="tok-k">int</span> u, <span class="tok-k">int</span> v, <span class="tok-t">T</span> w) {</span><span class="ln l-u">  adj[u].push_back({u, v, w});</span><span class="ln l-v">  adj[v].push_back({v, u, w});</span><span class="ln">  ++grau[u]; ++grau[v]; ++m;</span><span class="ln">}</span></pre>
          <div class="row">
            <button class="btn primary b-play">${ICON.play} construir</button>
            <button class="btn b-step">${ICON.next} passo</button>
            <button class="btn b-reset">${ICON.reset}</button>
          </div>
        </div>
        <div class="card"><h3>grafo · passe o mouse</h3><div class="gw-canvas readonly lib-canvas" style="flex:1;min-height:240px"></div>
          <div class="note lib-count"></div></div>
        <div class="card"><h3>vector&lt;vector&lt;Edge&lt;T&gt;&gt;&gt; adj</h3><div class="adj-list lib-adj"></div></div>
      </div>`;
    const q = (s) => root.querySelector(s);
    const view = new GraphView(q(".lib-canvas"), { showWeights: true, onHover: (v) => hl(v) });
    let step = 0, timer = null;
    const adjRows = () => q(".lib-adj");

    function render(animate) {
      const edges = base.edges.slice(0, Math.max(0, step - 1));
      const nodes = step >= 1 ? base.nodes : [];
      view.setGraph({ nodes, edges }, 1);
      root.querySelectorAll(".fl").forEach((f) => f.classList.toggle("on", +f.dataset.i === step - 1));
      root.querySelectorAll(".fl").forEach((f) => f.classList.toggle("done", +f.dataset.i < step - 1));
      const adj = Engine.buildAdj({ nodes, edges });
      adjRows().innerHTML = nodes.length
        ? adj.slice(1).map((arr, i) => `<div class="adj-row" data-v="${i + 1}"><span class="k">${i + 1} →</span>${arr.map((e) => {
          const fresh = animate && edges.length && e.id === edges[edges.length - 1].id;
          return `<span class="cell ${fresh ? "fresh" : ""}">${e.v}<i>, ${Engine.fmt(e.w)}</i></span>`;
        }).join("")}</div>`).join("")
        : `<span class="muted">vazio: a primeira linha ainda não foi lida</span>`;
      adjRows().querySelectorAll(".adj-row").forEach((r) => {
        r.addEventListener("mouseenter", () => hl(+r.dataset.v));
        r.addEventListener("mouseleave", () => hl(null));
      });
      const last = edges[edges.length - 1];
      root.querySelectorAll(".lib-code .l-u, .lib-code .l-v").forEach((l) => l.classList.toggle("hl", !!last && animate));
      if (last && animate) {
        view.gEdges.selectAll("g.g-edge").filter((d) => d.id === last.id).classed("active", true);
        view.gNodes.selectAll("g.g-node").filter((d) => d.id === last.u || d.id === last.v).classed("current", true);
      }
      q(".lib-count").innerHTML = step === 0 ? "aperte <b>construir</b>"
        : `n = <b>${nodes.length}</b> · m = <b>${edges.length}</b> arestas → <b>${2 * edges.length}</b> entradas na lista (cada aresta aparece nas duas pontas)`;
    }
    function hl(v) {
      if (step < linhas.length) return;
      adjRows().querySelectorAll(".adj-row").forEach((r) => r.classList.toggle("hover", +r.dataset.v === v));
      if (!v) { view.render(null); return; }
      const adj = Engine.buildAdj(view.graph);
      view.render(null, { highlightNodes: [v, ...adj[v].map((e) => e.v)], highlightEdges: adj[v].map((e) => e.id) });
    }
    const next = () => { if (step < linhas.length) { step++; render(true); } if (step >= linhas.length) stop(); };
    function play() {
      if (step >= linhas.length) { step = 0; render(false); }
      q(".b-play").innerHTML = `${ICON.pause} pausar`;
      timer = setInterval(next, 1100);
    }
    function stop() { clearInterval(timer); timer = null; q(".b-play").innerHTML = `${ICON.play} construir`; }
    q(".b-play").addEventListener("click", () => (timer ? stop() : play()));
    q(".b-step").addEventListener("click", () => { stop(); next(); });
    q(".b-reset").addEventListener("click", () => { stop(); step = 0; render(false); });
    render(false);
    return { enter() { if (step === 0) setTimeout(play, 500); }, leave: stop };
  }

  // =================================================================
  // Slide 3 (complemento): matriz esparsa acendendo celulas
  // =================================================================
  function SparseGrid(root) {
    const n = 28;
    root.innerHTML = `<svg class="sparse" viewBox="0 0 ${n * 12} ${n * 12}"></svg><div class="note sparse-note"></div>`;
    const svg = d3.select(root.querySelector("svg"));
    const cells = [];
    for (let i = 0; i < n; i++) for (let j = 0; j < n; j++) cells.push({ i, j });
    const rects = svg.selectAll("rect").data(cells).join("rect")
      .attr("x", (d) => d.j * 12 + 1).attr("y", (d) => d.i * 12 + 1).attr("width", 10).attr("height", 10).attr("rx", 2)
      .attr("class", "sc");
    let timer = null, on = new Set(), m = 0;
    function reset() { on = new Set(); m = 0; rects.classed("on", false).classed("diag", (d) => d.i === d.j); note(); }
    function note() {
      root.querySelector(".sparse-note").innerHTML = `${n} vértices, <b>${m}</b> arestas: só <b>${2 * m}</b> de <b>${n * n}</b> células usadas (${fNum((200 * m) / (n * n), 1)}%). O resto da matriz guarda zeros.`;
    }
    function add() {
      if (m >= n * 1.5) { clearInterval(timer); timer = setTimeout(() => { reset(); start(); }, 2600); return; }
      let u, v;
      do { u = Math.floor(Math.random() * n); v = Math.floor(Math.random() * n); } while (u === v || on.has(`${u},${v}`));
      on.add(`${u},${v}`); on.add(`${v},${u}`); m++;
      rects.filter((d) => (d.i === u && d.j === v) || (d.i === v && d.j === u)).classed("on", true);
      note();
    }
    function start() { clearInterval(timer); timer = setInterval(add, 120); }
    reset();
    return { enter() { reset(); start(); }, leave() { clearInterval(timer); clearTimeout(timer); } };
  }

  // =================================================================
  // Slide 6: as quatro fronteiras rodando ao mesmo tempo
  // =================================================================
  const QUAD = [
    { k: "bfs", regra: "sai quem <b>entrou primeiro</b>", estrutura: "Fila" },
    { k: "dfs", regra: "sai quem <b>entrou por último</b>", estrutura: "Pilha" },
    { k: "dij-vetor", regra: "sai o <b>mais perto</b>, procurando um por um", estrutura: "Vetor" },
    { k: "dij-heap", regra: "sai o <b>mais perto</b>, que já está no topo", estrutura: "Heap" },
  ];

  function QuadRun(root) {
    root.innerHTML = `
      <div class="quad-grid">
        <div class="card quad-left">
          <h3>o mesmo laço para as quatro</h3>
          <pre class="code quad-code">${CODE_LINES.map((l, i) => `<span class="ln" data-l="${i + 1}"><span class="marks">${QUAD.map((x) => `<i class="mk" data-k="${x.k}" style="background:${Engine.ALGS[x.k].cor}"></i>`).join("")}</span>${l}</span>`).join("")}</pre>
          <div class="rules">${QUAD.map((x) => `<div class="rule" data-k="${x.k}" style="--alg:${Engine.ALGS[x.k].cor}">
            <span class="sw" style="color:${Engine.ALGS[x.k].cor}"></span><b>${x.estrutura}</b><span>${x.regra}</span></div>`).join("")}</div>
          <div class="row">
            <button class="btn primary q-play">${ICON.play}</button>
            <button class="btn q-step">${ICON.next} passo</button>
            <button class="btn q-reset">${ICON.reset}</button>
            <select class="sel q-speed"><option value="0.5">0,5×</option><option value="1" selected>1×</option><option value="2">2×</option></select>
            <span class="spacer"></span><span class="mono muted q-info" style="font-size:12px"></span>
          </div>
        </div>
        <div class="quad-panels">${QUAD.map((x) => `
          <div class="qp" data-k="${x.k}" style="--alg:${Engine.ALGS[x.k].cor}">
            <div class="qp-head"><b>${Engine.ALGS[x.k].nome}</b><span class="qp-msg"></span></div>
            <div class="gw-canvas readonly qp-canvas"></div>
            <div class="qp-fr"><span class="cap">próximo a sair →</span><span class="qp-items"></span></div>
            <div class="qp-ord"><span class="cap">ordem de visita</span><span class="qp-order"></span></div>
          </div>`).join("")}</div>
      </div>`;
    const q = (s) => root.querySelector(s);
    const g = GraphGen.EXEMPLOS.dijkstra;
    const panels = QUAD.map((x) => {
      const p = root.querySelector(`.qp[data-k="${x.k}"]`);
      const view = new GraphView(p.querySelector(".qp-canvas"), { showWeights: Engine.ALGS[x.k].pesos });
      view.setGraph(g, 1);
      return { ...x, p, view, run: Engine.run(view.graph, 1, x.k), i: 0 };
    });
    let timer = null, speed = 1;

    function exitOrder(x, s) {
      const it = s.frontier;
      if (x.k === "bfs") return it.map((e) => e.vertex);
      if (x.k === "dfs") return it.slice().reverse().map((e) => e.vertex);
      if (x.k === "dij-vetor") return it.filter((e) => e.pending).sort((a, b) => a.cost - b.cost || a.vertex - b.vertex).map((e) => `${e.vertex}<small>${Engine.fmt(e.cost)}</small>`);
      return it.slice().sort((a, b) => a.cost - b.cost || a.vertex - b.vertex).map((e) => `${e.vertex}<small>${Engine.fmt(e.cost)}</small>`);
    }
    function render() {
      root.querySelectorAll(".quad-code .mk").forEach((m) => m.classList.remove("on"));
      for (const x of panels) {
        const s = x.run.steps[x.i];
        x.view.render(s);
        const mk = root.querySelector(`.quad-code .ln[data-l="${s.line}"] .mk[data-k="${x.k}"]`);
        if (mk) mk.classList.add("on");
        const ex = exitOrder(x, s);
        x.p.querySelector(".qp-items").innerHTML = ex.length
          ? ex.slice(0, 7).map((v, i) => `<span class="chip ${i === 0 ? "head" : ""}">${v}</span>`).join("") + (ex.length > 7 ? `<span class="cap">+${ex.length - 7}</span>` : "")
          : `<span class="cap">vazia</span>`;
        x.p.querySelector(".qp-order").textContent = s.order.join(" → ");
        x.p.querySelector(".qp-msg").textContent = { init: "início", pop: "pop", discard: "descarta", visit: "visita", explore: "explore", done: "fim" }[s.type];
        x.p.classList.toggle("popping", s.type === "pop");
        root.querySelector(`.rule[data-k="${x.k}"]`).classList.toggle("on", s.type === "pop");
      }
      q(".q-info").textContent = panels.map((x) => `${x.i + 1}/${x.run.steps.length}`).join(" · ");
    }
    const atEnd = () => panels.every((x) => x.i >= x.run.steps.length - 1);
    function step() {
      panels.forEach((x) => { x.i = Math.min(x.run.steps.length - 1, x.i + 1); });
      render();
      if (atEnd()) stop();
    }
    function play() {
      if (atEnd()) panels.forEach((x) => { x.i = 0; });
      q(".q-play").innerHTML = ICON.pause;
      timer = setInterval(step, 900 / speed);
    }
    function stop() { clearInterval(timer); timer = null; q(".q-play").innerHTML = ICON.play; }
    q(".q-play").addEventListener("click", () => (timer ? stop() : play()));
    q(".q-step").addEventListener("click", () => { stop(); step(); });
    q(".q-reset").addEventListener("click", () => { stop(); panels.forEach((x) => { x.i = 0; }); render(); });
    q(".q-speed").addEventListener("change", (e) => { speed = +e.target.value; if (timer) { stop(); play(); } });
    render();
    return { enter() { if (!timer && panels.every((x) => x.i === 0)) setTimeout(play, 600); }, leave: stop };
  }

  // =================================================================
  // Slide 8: vetor x heap com o grafo em 3D girando
  // =================================================================
  function heapTree(items, dist, rootEntry) {
    if (!items.length) return `<div class="chips"><span class="empty">vazio</span></div>`;
    const shown = items.slice(0, 31);
    const Wd = 340, rowH = 36;
    const depth = Math.floor(Math.log2(shown.length)) + 1;
    const pos = shown.map((_, i) => {
      const d = Math.floor(Math.log2(i + 1));
      const k = i + 1 - 2 ** d;
      return { x: ((k + 0.5) / 2 ** d) * Wd, y: 18 + d * rowH };
    });
    let out = `<svg class="heap-svg" viewBox="0 0 ${Wd} ${depth * rowH + 6}" style="height:${Math.min(190, depth * rowH + 6)}px">`;
    shown.forEach((_, i) => { if (i > 0) { const p = pos[(i - 1) >> 1]; out += `<line x1="${p.x}" y1="${p.y}" x2="${pos[i].x}" y2="${pos[i].y}"/>`; } });
    shown.forEach((x, i) => {
      const stale = x.cost !== dist[x.vertex];
      out += `<g class="hn ${i === 0 ? "root" : ""} ${stale ? "stale" : ""}" transform="translate(${pos[i].x},${pos[i].y})">
        <circle r="15"/><text y="-3">${x.vertex}</text><text class="c" y="8">${Engine.fmt(x.cost)}</text></g>`;
    });
    out += "</svg>";
    if (items.length > shown.length) out += `<div class="note">+${items.length - shown.length} entradas</div>`;
    return out;
  }

  function VecHeap3D(root) {
    root.innerHTML = `
      <div class="row">
        <button class="btn primary vh-play">${ICON.play}</button>
        <button class="btn vh-step">${ICON.next} próximo vértice</button>
        <button class="btn vh-reset">${ICON.reset}</button>
        <button class="btn vh-new">${ICON.dice} novo grafo</button>
        <span class="mono muted" style="font-size:12px">n =</span>
        <input type="range" class="vh-n" min="8" max="60" value="18" style="width:130px;accent-color:var(--dij)"/>
        <span class="mono vh-nv" style="font-size:13px">18</span>
        <span class="spacer"></span>
        <label class="mono muted" style="font-size:12px;display:flex;gap:6px;align-items:center"><input type="checkbox" class="vh-rot" checked/> girar</label>
      </div>
      <div class="vh-grid">
        <div class="card vh-3d-card"><div class="vh-3d"></div><div class="vh-labels"></div>
          <div class="vh-legend"><span><i style="background:var(--dij)"></i>finalizado</span><span><i style="background:#fff"></i>extraído agora</span><span><i style="background:var(--warn)"></i>na fronteira</span><span><i style="background:#3a4459"></i>não alcançado</span></div>
          <div class="vh-caption mono"></div></div>
        <div class="vh-side">
          <div class="card" style="--alg:var(--vec)">
            <div class="row"><b class="c-vec">Vetor</b><span class="muted" style="font-size:13px">olha os n vértices para achar o menor</span><span class="spacer"></span><span class="mono vh-vc c-vec"></span></div>
            <div class="vec-strip"></div>
          </div>
          <div class="card" style="--alg:var(--dij)">
            <div class="row"><b class="c-dij">Heap</b><span class="muted" style="font-size:13px">o menor já está no topo</span><span class="spacer"></span><span class="mono vh-hc c-dij"></span></div>
            <div class="vh-heap"></div>
          </div>
          <div class="card">
            <h3>comparações acumuladas</h3>
            <div class="race"><div class="race-row"><span>vetor</span><div class="race-track"><div class="race-fill vh-rv" style="background:var(--vec)"></div></div><b class="vh-rvn c-vec">0</b></div>
            <div class="race-row"><span>heap</span><div class="race-track"><div class="race-fill vh-rh" style="background:var(--dij)"></div></div><b class="vh-rhn c-dij">0</b></div></div>
          </div>
        </div>
      </div>`;
    const q = (s) => root.querySelector(s);
    let n = 18, rv, rh, vVisit, hVisit, k = 0, timer = null, G = null, cam = null, gdata = null, labelRaf = null;
    const box3d = q(".vh-3d");

    function build() {
      const g = GraphGen.randomGraph(n, Math.round(n * 0.8), true);
      rv = Engine.run(g, 1, "dij-vetor");
      rh = Engine.run(g, 1, "dij-heap");
      vVisit = rv.steps.map((s, i) => (s.type === "visit" ? i : -1)).filter((i) => i >= 0);
      hVisit = rh.steps.map((s, i) => (s.type === "visit" ? i : -1)).filter((i) => i >= 0);
      k = 0;
      gdata = {
        nodes: g.nodes.map((v) => ({ id: v.id })),
        links: g.edges.map((e) => ({ source: e.u, target: e.v, id: e.id, w: e.w })),
      };
      if (has3D) {
        if (!G) {
          G = ForceGraph3D({ controlType: "orbit" })(box3d)
            .backgroundColor("rgba(0,0,0,0)")
            .showNavInfo(false)
            .nodeRelSize(4.5)
            .nodeResolution(16)
            .nodeOpacity(1)
            .linkOpacity(0.9)
            .nodeLabel((d) => `vértice ${d.id}`)
            .d3VelocityDecay(0.3);
          fit3D(G, box3d);
          cam = orbit(G, () => 120 + n * 3.2, 0.003, 0.3);
          q(".vh-rot").addEventListener("change", (e) => cam.set(e.target.checked));
        }
        G.graphData(gdata);
      } else {
        box3d.innerHTML = `<div class="note" style="padding:20px">WebGL indisponível neste navegador: visualização 3D desligada.</div>`;
      }
      render();
    }

    const state = () => (k === 0 ? rh.steps[0] : rh.steps[hVisit[k - 1]]);

    function render() {
      const s = state();
      const visited = new Set(s.order);
      const cur = k > 0 ? s.order[k - 1] : null;
      const tree = new Set(s.parEdge.filter((x) => x != null));
      if (G) {
        G.nodeColor((d) => (d.id === cur ? "#ffffff" : visited.has(d.id) ? css("--dij") : s.frontierVerts.has(d.id) ? css("--warn") : "#3a4459"))
          .nodeVal((d) => (d.id === cur ? 5 : visited.has(d.id) ? 2 : 1))
          .linkColor((l) => (tree.has(l.id) ? (visited.has(l.target.id ?? l.target) ? css("--dij") : css("--warn")) : "#2f3850"))
          .linkWidth((l) => (tree.has(l.id) ? 1.6 : 0.4));
      }
      // vetor: a varredura desenhada celula a celula
      const sv = k === 0 ? rv.steps[0] : rv.steps[vVisit[k - 1] - 1];
      const strip = q(".vec-strip");
      strip.innerHTML = sv.frontier.map((x, i) => `<span class="vs ${x.pending || x.vertex === sv.minVertex ? "p" : ""} ${x.vertex === sv.minVertex && k > 0 ? "min" : ""}" style="--d:${i * (600 / n)}ms"><i>${x.vertex}</i>${x.cost === Infinity ? "∞" : Engine.fmt(x.cost)}</span>`).join("");
      if (k > 0) strip.classList.remove("scan"), void strip.offsetWidth, strip.classList.add("scan");
      // heap: estado antes do pop, com a raiz destacada
      const popIdx = k === 0 ? 0 : hVisit[k - 1] - 1;
      const before = k === 0 ? rh.steps[0] : rh.steps[Math.max(0, popIdx - 1)];
      q(".vh-heap").innerHTML = heapTree(before.frontier, before.dist);
      const cv = (k === 0 ? rv.steps[0] : rv.steps[vVisit[k - 1]]).counters;
      const ch = (k === 0 ? rh.steps[0] : rh.steps[hVisit[k - 1]]).counters;
      q(".vh-vc").textContent = `${fInt(cv.work)} comparações`;
      q(".vh-hc").textContent = `${fInt(ch.work)} comparações${ch.discards ? ` · ${ch.discards} descartes` : ""}`;
      const max = Math.max(1, rv.steps[rv.steps.length - 1].counters.work);
      q(".vh-rv").style.width = `${(100 * cv.work) / max}%`;
      q(".vh-rh").style.width = `${(100 * ch.work) / max}%`;
      q(".vh-rvn").textContent = fInt(cv.work);
      q(".vh-rhn").textContent = fInt(ch.work);
      q(".vh-caption").innerHTML = k === 0
        ? `origem 1 · ${n} vértices · aperte ▶`
        : `vértice <b>${cur}</b> finalizado (${k}/${vVisit.length}) · dist ${Engine.fmt(s.dist[cur])}`;
    }

    // rotulos HTML acompanhando os vertices 3D
    function labels() {
      const host = q(".vh-labels");
      if (!G) return;
      if (host.childElementCount !== gdata.nodes.length) host.innerHTML = gdata.nodes.map((d) => `<span data-id="${d.id}">${d.id}</span>`).join("");
      const spans = host.children;
      gdata.nodes.forEach((d, i) => {
        if (d.x == null) return;
        const p = G.graph2ScreenCoords(d.x, d.y, d.z);
        spans[i].style.transform = `translate(${p.x}px, ${p.y}px)`;
      });
      labelRaf = requestAnimationFrame(labels);
    }

    const next = () => { if (k < vVisit.length) { k++; render(); } if (k >= vVisit.length) stop(); };
    function play() {
      if (k >= vVisit.length) { k = 0; render(); }
      q(".vh-play").innerHTML = ICON.pause;
      timer = setInterval(next, 1300);
    }
    function stop() { clearInterval(timer); timer = null; q(".vh-play").innerHTML = ICON.play; }
    q(".vh-play").addEventListener("click", () => (timer ? stop() : play()));
    q(".vh-step").addEventListener("click", () => { stop(); next(); });
    q(".vh-reset").addEventListener("click", () => { stop(); k = 0; render(); });
    q(".vh-new").addEventListener("click", () => { stop(); build(); });
    q(".vh-n").addEventListener("input", (e) => { n = +e.target.value; q(".vh-nv").textContent = n; stop(); build(); });
    build();
    return {
      enter() { if (G) { G.resumeAnimation(); cam.start(); labels(); } if (k === 0) setTimeout(play, 900); },
      leave() { stop(); if (G) { cam.stop(); G.pauseAnimation(); cancelAnimationFrame(labelRaf); } },
    };
  }

  // =================================================================
  // Slide 12: arvore real crescendo por layout de forca
  // =================================================================
  const EXPLICA = {
    bfs: (i) => `<p><b class="c-bfs">BFS · fila.</b> Sai quem entrou primeiro. Ao visitar a origem, <b>todos os ${i.filhosRaiz} vizinhos</b> entram na fila antes de qualquer outro vértice,
      e só depois deles aparecem os vizinhos dos vizinhos. Por isso ela se abre <b>em volta da origem</b>, um anel (nível) de cada vez: fica <b>larga e rasa</b>.</p>`,
    dfs: (i) => `<p><b class="c-dfs">DFS · pilha.</b> Sai quem entrou por último, que é sempre um vizinho do vértice que acabou de ser visitado.
      Assim ela <b>nunca volta</b> enquanto encontra vizinho novo, e cada vértice visitado é filho do anterior. O resultado é um <b>caminho comprido</b>: ${i.total} vértices e profundidade ${i.profundidade}.</p>`,
    dijkstra: (i) => `<p><b class="c-dij">Dijkstra · heap.</b> Sai quem está mais perto pela <b>soma dos pesos</b>. Um vizinho ligado por uma aresta pesada espera,
      enquanto um "neto" alcançado por arestas leves passa na frente. Fica entre as duas: mais funda que a BFS (profundidade ${i.profundidade}) e bem mais larga que a DFS.</p>`,
  };

  function ForceTree(container, onInfo) {
    const svg = d3.select(container).append("svg").attr("class", "tree-svg");
    let sim = null, adder = null, fitTimer = null, size = [600, 500];
    const COL = { bfs: "--bfs", dfs: "--dfs", dijkstra: "--dij" };

    function show(gid, alg) {
      clearInterval(adder);
      clearTimeout(fitTimer);
      if (sim) sim.stop();
      svg.selectAll("*").remove();
      const r = container.getBoundingClientRect();
      if (r.width > 10 && r.height > 10) size = [r.width, r.height];
      const [w, h] = size;
      const rows = DADOS.arvores[gid][alg];
      const col = css(COL[alg]);
      const depth = new Map();
      rows.forEach((r) => depth.set(r[0], r[1] === r[0] ? 0 : (depth.get(r[1]) ?? 0) + 1));
      const all = rows.map((r) => ({ id: r[0], pai: r[1], dist: r[2], ordem: r[3], depth: depth.get(r[0]) }));
      const byId = new Map(all.map((d) => [d.id, d]));
      const maxD = d3.max(all, (d) => d.depth);
      onInfo({
        total: all.length,
        profundidade: maxD,
        filhosRaiz: all.filter((d) => d.pai === all[0].id && d.id !== d.pai).length,
        folhas: all.filter((d) => !all.some((c) => c.pai === d.id && c.id !== c.pai)).length,
      });
      const color = d3.scaleLinear().domain([0, Math.max(1, maxD)]).range([col, d3.color(col).darker(1.6).formatHex()]);
      const g = svg.append("g");
      const zoom = d3.zoom().scaleExtent([0.2, 5]).on("zoom", (e) => g.attr("transform", e.transform));
      svg.call(zoom).on("dblclick.zoom", null);
      const linkG = g.append("g"), nodeG = g.append("g");
      const nodes = [], links = [];
      all[0].fx = w / 2; all[0].fy = h / 2;
      sim = d3.forceSimulation(nodes)
        .force("link", d3.forceLink(links).id((d) => d.id).distance(alg === "dfs" ? 9 : 16).strength(1))
        .force("charge", d3.forceManyBody().strength(alg === "dfs" ? -14 : -26))
        .force("x", d3.forceX(w / 2).strength(0.012))
        .force("y", d3.forceY(h / 2).strength(0.012))
        .on("tick", draw);
      let i = 0;
      adder = setInterval(() => {
        for (let b = 0; b < 3 && i < all.length; b++, i++) {
          const d = all[i];
          if (d.pai !== d.id) {
            const p = byId.get(d.pai);
            d.x = p.x + (Math.random() - 0.5) * 10; d.y = p.y + (Math.random() - 0.5) * 10;
            links.push({ source: d.id, target: d.pai });
          }
          nodes.push(d);
        }
        sim.nodes(nodes);
        sim.force("link").links(links);
        sim.alpha(0.6).restart();
        if (i >= all.length) { clearInterval(adder); fitTimer = setTimeout(fit, 900); sim.on("end", fit); }
      }, 28);
      // enquadra a arvore inteira no cartao
      function fit() {
        const xs = d3.extent(nodes, (d) => d.x), ys = d3.extent(nodes, (d) => d.y);
        const bw = Math.max(40, xs[1] - xs[0]), bh = Math.max(40, ys[1] - ys[0]);
        const k = Math.min(0.88 * w / bw, 0.88 * h / bh, 3);
        const t = d3.zoomIdentity.translate(w / 2 - k * (xs[0] + xs[1]) / 2, h / 2 - k * (ys[0] + ys[1]) / 2).scale(k);
        svg.transition().duration(900).call(zoom.transform, t);
      }
      function draw() {
        linkG.selectAll("line").data(links).join("line").attr("stroke", col).attr("stroke-opacity", 0.45).attr("stroke-width", 1.2)
          .attr("x1", (d) => d.source.x).attr("y1", (d) => d.source.y).attr("x2", (d) => d.target.x).attr("y2", (d) => d.target.y);
        nodeG.selectAll("circle").data(nodes, (d) => d.id).join((en) => en.append("circle")
          .attr("r", (d) => (d.depth === 0 ? 8 : 3.4)).attr("fill", (d) => (d.depth === 0 ? "#fff" : color(d.depth)))
          .on("mousemove", (ev, d) => Charts.showTip(ev, `vértice <b>${fInt(d.id)}</b><br>pai ${fInt(d.pai)} · profundidade ${d.depth}<br>${alg === "dijkstra" ? `dist <b>${fNum(d.dist, 3)}</b> · ` : ""}visita #${d.ordem}`))
          .on("mouseleave", Charts.hideTip))
          .attr("cx", (d) => d.x).attr("cy", (d) => d.y);
      }
    }
    new ResizeObserver(() => {
      const r = container.getBoundingClientRect();
      if (r.width > 10) size = [r.width, r.height];
    }).observe(container);
    return { show, stop() { clearInterval(adder); clearTimeout(fitTimer); if (sim) sim.stop(); } };
  }

  // =================================================================
  // Slide 13: explorador 3D das arvores reais (BFS + DFS + Dijkstra)
  // =================================================================
  function Explorer3D(container, onStats) {
    if (!has3D) {
      container.innerHTML = `<div class="note" style="padding:20px">WebGL indisponível: use a aba de imagens.</div>`;
      return { show() {}, enter() {}, leave() {} };
    }
    const COLS = { bfs: css("--bfs"), dfs: css("--dfs"), dijkstra: css("--dij") };
    const G = ForceGraph3D({ controlType: "orbit" })(container)
      .backgroundColor("rgba(0,0,0,0)")
      .showNavInfo(false)
      .nodeRelSize(2.2)
      .nodeResolution(8)
      .nodeOpacity(0.95)
      .linkOpacity(0.75)
      .linkWidth(0)
      .warmupTicks(40)
      .cooldownTime(9000)
      .nodeLabel((d) => `vértice ${d.id}${d.root ? " (origem)" : ""}<br>${[...d.algs].join(", ")}`);
    fit3D(G, container);
    // caminhos longos (DFS) com repulsao forte se espalham demais: forcas mais curtas
    G.d3Force("charge").strength(-6).distanceMax(120);
    G.d3Force("link").distance(5);
    let radius = 400, alvo = 400;
    // a camera acompanha o tamanho real da nuvem de vertices
    G.onEngineTick(() => {
      let r2 = 0;
      for (const d of G.graphData().nodes) r2 = Math.max(r2, (d.x || 0) ** 2 + (d.y || 0) ** 2 + (d.z || 0) ** 2);
      alvo = Math.max(80, Math.sqrt(r2) * 1.55);
    });
    const cam = orbit(G, () => (radius += (alvo - radius) * 0.04), 0.0016, 0.35);

    function show(gid, algs, limite) {
      const nodeMap = new Map();
      const edgeMap = new Map();
      for (const alg of algs) {
        const flat = DADOS3D.arvores[gid][alg];
        const lim = Math.min(limite, flat.length / 2);
        for (let i = 0; i < lim; i++) {
          const v = flat[2 * i], p = flat[2 * i + 1];
          if (!nodeMap.has(v)) nodeMap.set(v, { id: v, algs: new Set(), root: v === p });
          nodeMap.get(v).algs.add(alg);
          if (v !== p) {
            const key = v < p ? `${v}-${p}` : `${p}-${v}`;
            if (!edgeMap.has(key)) edgeMap.set(key, { source: v, target: p, algs: new Set() });
            edgeMap.get(key).algs.add(alg);
          }
        }
      }
      const nodes = [...nodeMap.values()];
      const links = [...edgeMap.values()];
      const shared = links.filter((l) => l.algs.size > 1).length;
      G.nodeColor((d) => (d.root ? "#ffffff" : d.algs.size > 1 ? "#c9d2e3" : COLS[[...d.algs][0]]))
        .nodeVal((d) => (d.root ? 12 : 1))
        .linkColor((l) => (l.algs.size > 1 ? "#ffffff" : COLS[[...l.algs][0]]))
        .graphData({ nodes, links });
      onStats && onStats({ nodes: nodes.length, links: links.length, shared, algs });
    }
    return {
      show,
      enter() { G.resumeAnimation(); cam.start(); },
      leave() { cam.stop(); G.pauseAnimation(); },
      rotate(v) { cam.set(v); },
    };
  }

  // =================================================================
  // Estudo 3: os caminhos a partir de Dijkstra como um grafo de nomes
  // =================================================================
  function ResearchGraph(container, estudo) {
    const svg = d3.select(container).append("svg");
    const nodes = new Map(), links = new Map();
    const alvo = new Set(estudo.destinos.map((d) => d.destino));
    estudo.destinos.forEach((d) => {
      if (!d.caminho) return;
      d.caminho.forEach((nome, i) => {
        if (!nodes.has(nome)) nodes.set(nome, { id: nome, tipo: i === 0 ? "origem" : alvo.has(nome) ? "alvo" : "meio", primeiro: Infinity });
        nodes.get(nome).primeiro = Math.min(nodes.get(nome).primeiro, i);
        if (i) {
          const a = d.caminho[i - 1];
          const key = [a, nome].sort().join("|");
          if (!links.has(key)) links.set(key, { source: a, target: nome, rotas: new Set() });
          links.get(key).rotas.add(d.destino);
        }
      });
    });
    // destinos sem caminho: desenhados como uma ilha separada (outra componente)
    const ilhas = estudo.destinos.filter((d) => !d.caminho && d.coautores);
    ilhas.forEach((d) => {
      nodes.set(d.destino, { id: d.destino, tipo: "alvo", primeiro: 0, ilha: true });
      d.coautores.forEach((c) => {
        if (!nodes.has(c)) nodes.set(c, { id: c, tipo: "meio", primeiro: 1, ilha: true });
        links.set(`${c}|${d.destino}`, { source: c, target: d.destino, rotas: new Set([d.destino]) });
      });
    });
    const N = [...nodes.values()], L = [...links.values()];
    const rotas = estudo.destinos.filter((d) => d.caminho);
    N.forEach((n) => {
      const fs = rotas.map((d, r) => (d.caminho.includes(n.id) ? r / Math.max(1, rotas.length - 1) : null)).filter((f) => f != null);
      n.faixa = d3.mean(fs) ?? 1;
    });
    let sim = null, hoverAlvo = null, size = [600, 400], started = false;

    function draw(w, h) {
      size = [w, h];
      svg.selectAll("*").remove();
      if (sim) sim.stop();
      // "mapa de metro": x = passo no caminho, y = faixa media das rotas que passam no vertice
      const origem = N.find((d) => d.tipo === "origem");
      const passos = d3.max(N, (d) => d.primeiro) || 1;
      const X = (i) => w * 0.1 + i * ((w * 0.7) / passos);
      const Y = (f) => h * 0.08 + f * h * (ilhas.length ? 0.66 : 0.8);
      if (origem) { origem.fx = X(0); origem.fy = Y(origem.faixa); }
      N.forEach((d) => { d.x = X(d.primeiro); d.y = Y(d.faixa); });
      // ilha no canto inferior direito, presa no lugar
      const ilhaN = N.filter((d) => d.ilha).sort((a, b) => (a.tipo === "alvo") - (b.tipo === "alvo"));
      ilhaN.forEach((d, j) => { d.fx = w * 0.66 + j * 110; d.fy = h - 40; });
      if (ilhaN.length) {
        svg.append("rect").attr("class", "rg-ilha").attr("x", w * 0.66 - 60).attr("y", h - 78).attr("width", 110 * (ilhaN.length - 1) + 200).attr("height", 64).attr("rx", 10);
        svg.append("text").attr("class", "rg-ilha-t").attr("x", w * 0.66 - 56).attr("y", h - 86).text("outra componente: sem caminho a partir de Dijkstra");
      }
      sim = d3.forceSimulation(N)
        .force("link", d3.forceLink(L).id((d) => d.id).distance(40).strength(0.25))
        .force("x", d3.forceX((d) => X(d.primeiro)).strength(0.9))
        .force("y", d3.forceY((d) => Y(d.faixa)).strength(0.5))
        .force("collide", d3.forceCollide(16));
      const lk = svg.append("g").selectAll("line").data(L).join("line").attr("class", "rg-link");
      const nd = svg.append("g").selectAll("g").data(N).join("g").attr("class", (d) => `rg-node ${d.tipo}`);
      nd.append("circle").attr("r", (d) => (d.tipo === "meio" ? 6 : 10));
      // rotulos alternando acima/abaixo para nao encavalarem; alvos e origem ao lado
      nd.append("text")
        .attr("dy", (d) => (d.tipo === "meio" ? (d.primeiro % 2 ? 20 : -11) : d.tipo === "origem" ? -17 : "0.35em"))
        .attr("dx", (d) => (d.tipo === "alvo" ? 15 : 0))
        .attr("text-anchor", (d) => (d.tipo === "alvo" ? "start" : "middle"))
        .text((d) => d.id);
      nd.on("mouseenter", (ev, d) => { hoverAlvo = d.tipo === "alvo" ? d.id : null; paint(); })
        .on("mouseleave", () => { hoverAlvo = null; paint(); });
      function paint() {
        lk.classed("hl", (l) => !!hoverAlvo && l.rotas.has(hoverAlvo)).classed("dim", (l) => !!hoverAlvo && !l.rotas.has(hoverAlvo));
      }
      sim.on("tick", () => {
        N.forEach((d) => { d.x = Math.max(60, Math.min(w - 70, d.x)); d.y = Math.max(24, Math.min(h - 14, d.y)); });
        lk.attr("x1", (d) => d.source.x).attr("y1", (d) => d.source.y).attr("x2", (d) => d.target.x).attr("y2", (d) => d.target.y);
        nd.attr("transform", (d) => `translate(${d.x},${d.y})`);
      });
      container.highlight = (nome) => { hoverAlvo = nome; paint(); };
    }
    new ResizeObserver(() => {
      const r = container.getBoundingClientRect();
      if (r.width > 10 && (!started || Math.abs(r.width - size[0]) > 40)) { started = true; draw(r.width, r.height); }
    }).observe(container);
  }

  window.Scenes = { Cover3D, LibBuild, SparseGrid, QuadRun, VecHeap3D, ForceTree, Explorer3D, ResearchGraph, EXPLICA, has3D };
})();
