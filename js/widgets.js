/* Widget "algoritmo ao vivo": canvas editavel + fronteira + tabela + controles. */
(function () {
  const ICON = {
    first: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 20 9 12l10-8v16Z"/><path d="M5 19V5"/></svg>',
    prev: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m15 18-6-6 6-6"/></svg>',
    next: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m9 18 6-6-6-6"/></svg>',
    last: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m5 4 10 8-10 8V4Z"/><path d="M19 5v14"/></svg>',
    play: '<svg viewBox="0 0 24 24" fill="currentColor"><path d="M7 4.5v15a1 1 0 0 0 1.5.86l12.5-7.5a1 1 0 0 0 0-1.72L8.5 3.64A1 1 0 0 0 7 4.5Z"/></svg>',
    pause: '<svg viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="4" width="4" height="16" rx="1"/><rect x="14" y="4" width="4" height="16" rx="1"/></svg>',
    move: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 9l-3 3 3 3M9 5l3-3 3 3M15 19l-3 3-3-3M19 9l3 3-3 3M2 12h20M12 2v20"/></svg>',
    node: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="7"/><path d="M12 9v6M9 12h6"/></svg>',
    edge: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="5" cy="19" r="3"/><circle cx="19" cy="5" r="3"/><path d="M7.5 16.5l9-9"/></svg>',
    trash: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 6h18M8 6V4h8v2M6 6l1 14h10l1-14"/></svg>',
    dice: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="4"/><circle cx="8.5" cy="8.5" r="1.2" fill="currentColor"/><circle cx="15.5" cy="15.5" r="1.2" fill="currentColor"/><circle cx="12" cy="12" r="1.2" fill="currentColor"/></svg>',
    reset: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 12a9 9 0 1 0 3-6.7L3 8"/><path d="M3 3v5h5"/></svg>',
  };

  const CODE_LINES = [
    '<span class="tok-f">frontier</span>.push(<span class="tok-n">0</span>, s, s);',
    '<span class="tok-k">while</span> (!<span class="tok-f">frontier</span>.empty()) {',
    '  <span class="tok-k">auto</span> e = <span class="tok-f">frontier</span>.pop();',
    '  <span class="tok-k">if</span> (!<span class="tok-f">frontier</span>.process(e, pai, dist))',
    '    <span class="tok-k">continue</span>;',
    '  ordem.push_back(e.vertex);',
    '  <span class="tok-k">for</span> (<span class="tok-k">const</span> <span class="tok-t">Edge</span>&lt;<span class="tok-t">T</span>&gt;&amp; aresta : adj[e.vertex])',
    '    <span class="tok-f">frontier</span>.explore(e.vertex, aresta, pai, dist);',
    '}',
  ];

  const FRONTIER_INFO = {
    bfs: "<b>Fila</b> (FIFO). <code>process</code>: se o vértice ainda não tem pai, define pai e nível = nível do pai + 1. <code>explore</code>: enfileira vizinhos sem pai.",
    dfs: "<b>Pilha</b> (LIFO). Mesmo <code>process</code>/<code>explore</code> da BFS; só a ordem de saída muda e a busca mergulha fundo.",
    "dij-heap": "<b>Heap binário</b>. <code>pop</code> em O(log n). <code>explore</code> relaxa e insere uma nova entrada; <code>process</code> descarta entradas obsoletas (custo ≠ dist).",
    "dij-vetor": "<b>Vetor de estimativas</b>. <code>pop</code> varre os n vértices: O(n). <code>explore</code> relaxa e sobrescreve a estimativa no lugar, sem duplicatas.",
  };

  function el(html) {
    const t = document.createElement("template");
    t.innerHTML = html.trim();
    return t.content.firstElementChild;
  }

  /**
   * opts: {
   *   graph, start, alg, algs (lista de chaves selecionaveis ou null),
   *   editable, showCode, presets: [{label, graph()}], speed
   * }
   */
  function AlgoWidget(root, opts) {
    const o = Object.assign({ start: 1, algs: null, editable: true, showCode: false, presets: [], speed: 1 }, opts);
    let alg = o.alg;
    let result = null, idx = 0, timer = null, speed = o.speed;

    const algButtons = (o.algs || []).map((k) =>
      `<button data-alg="${k}"><span class="sw" style="color:${Engine.ALGS[k].cor}"></span>${Engine.ALGS[k].nome}</button>`
    ).join("");

    const node = el(`
      <div class="gw">
        <div class="gw-toolbar">
          ${o.algs ? `<div class="seg algs">${algButtons}</div>${o.editable ? '<span class="sep"></span>' : ""}` : ""}
          ${o.editable ? `
          <div class="seg modes">
            <button data-mode="move" class="on" title="Arrastar vértices; clique define a origem">${ICON.move}Mover</button>
            <button data-mode="add-node" title="Clique no fundo para criar vértice">${ICON.node}Vértice</button>
            <button data-mode="add-edge" title="Clique em dois vértices">${ICON.edge}Aresta</button>
            <button data-mode="delete" title="Clique num vértice ou aresta para apagar">${ICON.trash}Apagar</button>
          </div>` : ""}
          <span class="spacer"></span>
          ${o.presets.map((p, i) => `<button class="btn" data-preset="${i}">${p.icon ? ICON[p.icon] : ""}${p.label}</button>`).join("")}
        </div>
        <div class="gw-main">
          <div class="gw-canvas ${o.editable ? "" : "readonly"}">
            <div class="gw-alert"><span class="txt"></span><button class="btn fix">Trocar por |peso|</button></div>
            <div class="gw-hint"></div>
          </div>
          <aside class="gw-side">
            <div class="panel"><div class="panel-title"><span>Passo</span><b class="algname"></b></div><div class="msg"></div></div>
            <div class="panel fr"><div class="panel-title"><span class="frname"></span><b class="frsize"></b></div><div class="frbody"></div></div>
            ${o.showCode ? `<div class="panel"><div class="panel-title"><span>traverse_impl (um laço só)</span></div><pre class="code codebox"></pre><div class="note frinfo"></div></div>` : ""}
            <div class="panel tblp"><div class="panel-title"><span>pai · dist · ordem</span><b class="counters"></b></div><table class="tbl"><thead><tr><th>v</th><th>pai</th><th class="dh">dist</th><th>ordem</th></tr></thead><tbody></tbody></table></div>
          </aside>
        </div>
        <div class="gw-controls">
          <button class="btn" data-act="first" title="Início">${ICON.first}</button>
          <button class="btn" data-act="prev" title="Passo anterior">${ICON.prev}</button>
          <button class="btn primary" data-act="play" title="Reproduzir / pausar">${ICON.play}</button>
          <button class="btn" data-act="next" title="Próximo passo">${ICON.next}</button>
          <button class="btn" data-act="last" title="Fim">${ICON.last}</button>
          <input type="range" class="scrub" min="0" max="0" value="0" />
          <span class="stepinfo"></span>
          <select class="sel speed" title="Velocidade">
            <option value="0.5">0,5×</option><option value="1">1×</option><option value="2">2×</option><option value="4">4×</option>
          </select>
          <select class="sel origin" title="Vértice de origem"></select>
        </div>
      </div>`);
    root.appendChild(node);

    const $ = (s) => node.querySelector(s);
    const canvas = $(".gw-canvas");
    const view = new GraphView(canvas, {
      showWeights: Engine.ALGS[alg].pesos,
      editable: o.editable,
      onChange: () => recompute(),
      onOrigin: (v) => { originSel.value = v; recompute(); },
      onHover: (v) => hover(v),
    });
    const originSel = $(".origin");
    const scrub = $(".scrub");
    $(".speed").value = String(speed);

    if (o.showCode) {
      $(".codebox").innerHTML = CODE_LINES.map((l, i) => `<span class="ln" data-l="${i + 1}">${l}</span>`).join("");
    }

    // ------------------------------------------------------------ eventos
    node.querySelectorAll("[data-alg]").forEach((b) => b.addEventListener("click", () => setAlg(b.dataset.alg)));
    node.querySelectorAll("[data-mode]").forEach((b) => b.addEventListener("click", () => {
      node.querySelectorAll("[data-mode]").forEach((x) => x.classList.toggle("on", x === b));
      view.setMode(b.dataset.mode);
      updateHint();
    }));
    node.querySelectorAll("[data-preset]").forEach((b) => b.addEventListener("click", () => {
      const p = o.presets[+b.dataset.preset];
      view.setGraph(p.graph(), 1);
      recompute();
    }));
    node.querySelector("[data-act=first]").addEventListener("click", () => go(0));
    node.querySelector("[data-act=prev]").addEventListener("click", () => go(idx - 1));
    node.querySelector("[data-act=next]").addEventListener("click", () => go(idx + 1));
    node.querySelector("[data-act=last]").addEventListener("click", () => go(steps().length - 1));
    node.querySelector("[data-act=play]").addEventListener("click", () => toggle());
    scrub.addEventListener("input", () => { stop(); go(+scrub.value); });
    $(".speed").addEventListener("change", (e) => { speed = +e.target.value; if (timer) { stop(); play(); } });
    originSel.addEventListener("change", () => { view.start = +originSel.value; recompute(); });
    $(".fix").addEventListener("click", () => {
      view.graph.edges.forEach((e) => { e.w = Math.abs(e.w); });
      view.draw();
      recompute();
    });

    // ------------------------------------------------------------ logica
    const steps = () => (result ? result.steps : []);

    function setAlg(k) {
      alg = k;
      node.style.setProperty("--alg", Engine.ALGS[k].cor);
      node.querySelectorAll("[data-alg]").forEach((b) => b.classList.toggle("on", b.dataset.alg === k));
      view.opts.showWeights = Engine.ALGS[k].pesos;
      view.draw();
      recompute();
    }

    function recompute() {
      stop();
      const n = view.graph.nodes.length;
      if (view.start > n) view.start = Math.max(1, n);
      originSel.innerHTML = d3.range(1, n + 1).map((v) => `<option value="${v}">origem ${v}</option>`).join("");
      originSel.value = view.start;
      result = Engine.run(view.graph, view.start, alg);
      const alert = $(".gw-alert");
      if (result.error === "negativo") {
        alert.classList.add("show");
        alert.querySelector(".txt").innerHTML =
          `<b>Peso negativo detectado.</b> A biblioteca ainda não implementa caminhos mínimos com pesos negativos (o Dijkstra exige pesos ≥ 0).`;
      } else {
        alert.classList.remove("show");
      }
      scrub.max = Math.max(0, steps().length - 1);
      $(".algname").textContent = Engine.ALGS[alg].nome;
      $(".frname").textContent = `Fronteira: ${Engine.ALGS[alg].fronteira}`;
      $(".dh").textContent = Engine.ALGS[alg].pesos ? "dist" : "nível";
      if (o.showCode) $(".frinfo").innerHTML = FRONTIER_INFO[alg];
      go(0);
    }

    function go(i) {
      const S = steps();
      if (!S.length) {
        idx = 0;
        view.render(null, { negEdges: result && result.negEdges });
        $(".msg").innerHTML = result && result.error ? "Corrija os pesos negativos para rodar o Dijkstra." : "Grafo vazio: crie vértices no modo <b>Vértice</b>.";
        $(".frbody").innerHTML = "";
        $(".tbl tbody").innerHTML = "";
        $(".stepinfo").textContent = "—";
        $(".frsize").textContent = "";
        $(".counters").textContent = "";
        return;
      }
      idx = Math.max(0, Math.min(S.length - 1, i));
      scrub.value = idx;
      const s = S[idx];
      view.render(s, hoverExtra(s));
      $(".msg").innerHTML = s.msg;
      $(".stepinfo").textContent = `${idx + 1} / ${S.length}`;
      renderFrontier(s);
      renderTable(s, S[idx - 1]);
      if (o.showCode) {
        node.querySelectorAll(".codebox .ln").forEach((ln) => {
          const l = +ln.dataset.l;
          ln.classList.toggle("hl", l === s.line || (s.type === "discard" && l === 4));
        });
      }
      if (idx === S.length - 1) stop();
    }

    function renderFrontier(s) {
      const body = $(".frbody");
      const kind = Engine.ALGS[alg].fronteira;
      const items = s.frontier;
      if (kind === "Vetor") {
        const pend = items.filter((x) => x.pending).length;
        $(".frsize").textContent = `${pend} pendente${pend === 1 ? "" : "s"}`;
        body.innerHTML = `<div class="vec-grid">${items.map((x) => `
          <div class="vec-cell ${x.pending ? "pending" : ""} ${s.minVertex === x.vertex ? "min" : ""}">
            <span class="i">${x.vertex}</span>${x.cost === Infinity ? "∞" : Engine.fmt(x.cost)}
          </div>`).join("")}</div>
          <div class="note">custo acumulado de extrair mínimos: <b>${s.counters.work}</b> comparações</div>`;
        return;
      }
      $(".frsize").textContent = `${items.length} ${items.length === 1 ? "item" : "itens"}`;
      if (kind === "Heap") {
        body.innerHTML = heapSVG(items, s) + `<div class="note">custo acumulado no heap: <b>${s.counters.work}</b> comparações · descartes: <b>${s.counters.discards}</b></div>`;
        return;
      }
      if (!items.length) { body.innerHTML = `<div class="chips"><span class="empty">vazia</span></div>`; return; }
      const stale = (x) => x.vertex !== x.parent && s.par[x.vertex] !== 0;
      if (kind === "Fila") {
        body.innerHTML = `<div class="chips"><span class="cap">frente →</span>${items.map((x, i) =>
          `<span class="chip ${i === 0 ? "head" : ""} ${stale(x) ? "stale" : ""}">${x.vertex}<small>de ${x.parent}</small></span>`).join("")}</div>`;
      } else {
        body.innerHTML = `<span class="cap mono muted" style="font-size:11px">topo ↓</span><div class="chips stack">${items.map((x, i) =>
          `<span class="chip ${i === items.length - 1 ? "head" : ""} ${stale(x) ? "stale" : ""}">${x.vertex}<small>de ${x.parent}</small></span>`).join("")}</div>`;
      }
    }

    function heapSVG(items, s) {
      if (!items.length) return `<div class="chips"><span class="empty">vazio</span></div>`;
      const shown = items.slice(0, 31);
      const Wd = 320, rowH = 34;
      const depth = Math.floor(Math.log2(shown.length)) + 1;
      const pos = shown.map((_, i) => {
        const d = Math.floor(Math.log2(i + 1));
        const k = i + 1 - 2 ** d;
        return { x: ((k + 0.5) / 2 ** d) * Wd, y: 18 + d * rowH };
      });
      let out = `<svg class="heap-svg" viewBox="0 0 ${Wd} ${depth * rowH + 8}" style="height:${depth * rowH + 8}px">`;
      shown.forEach((_, i) => {
        if (i > 0) { const p = pos[(i - 1) >> 1]; out += `<line x1="${p.x}" y1="${p.y}" x2="${pos[i].x}" y2="${pos[i].y}"/>`; }
      });
      shown.forEach((x, i) => {
        const stale = x.cost !== s.dist[x.vertex];
        out += `<g class="hn ${i === 0 ? "root" : ""} ${stale ? "stale" : ""}" transform="translate(${pos[i].x},${pos[i].y})">
          <circle r="14"/><text y="-3">${x.vertex}</text><text class="c" y="8">${Engine.fmt(x.cost)}</text></g>`;
      });
      out += "</svg>";
      if (items.length > shown.length) out += `<div class="note">+${items.length - shown.length} entradas</div>`;
      return out;
    }

    function renderTable(s, prev) {
      const n = s.par.length - 1;
      const rows = [];
      for (let v = 1; v <= n; v++) {
        const k = s.order.indexOf(v);
        const ch = prev && (prev.par[v] !== s.par[v] || prev.dist[v] !== s.dist[v]);
        rows.push(`<tr class="${s.current === v ? "cur" : ""} ${k >= 0 ? "done" : ""}">
          <td>${v}</td><td class="${ch ? "changed" : ""}">${s.par[v] || "—"}</td>
          <td class="${ch ? "changed" : ""}">${Engine.fmt(s.dist[v])}</td><td>${k >= 0 ? k + 1 : ""}</td></tr>`);
      }
      $(".tbl tbody").innerHTML = rows.join("");
      const c = s.counters;
      $(".counters").textContent = `push ${c.pushes} · pop ${c.pops}${c.discards ? ` · descartes ${c.discards}` : ""}`;
    }

    // caminho da origem ate o vertice sob o mouse
    let hovered = null;
    function hoverExtra(s) {
      if (!hovered || !s) return {};
      const p = Engine.pathTo(s.par, hovered);
      if (!p) return {};
      const edges = p.slice(1).map((v) => s.parEdge[v]);
      return { pathNodes: p, pathEdges: edges };
    }
    function hover(v) {
      hovered = v;
      const S = steps();
      if (!S.length) return;
      const s = S[idx];
      view.render(s, hoverExtra(s));
      updateHint(s);
    }
    function updateHint(s) {
      const hint = $(".gw-hint");
      if (hovered && s) {
        const p = Engine.pathTo(s.par, hovered);
        if (p) { hint.innerHTML = `caminho: <b>${p.join(" → ")}</b> · dist ${Engine.fmt(s.dist[hovered])}`; return; }
      }
      const tips = {
        move: o.editable ? "arraste os vértices · clique num vértice para torná-lo a origem · passe o mouse para ver o caminho" : "passe o mouse num vértice para ver o caminho até a origem",
        "add-node": "clique no fundo para criar um vértice",
        "add-edge": "clique em dois vértices para ligá-los",
        delete: "clique num vértice ou aresta para apagar",
      };
      hint.innerHTML = tips[view.mode] + (o.editable && Engine.ALGS[alg].pesos ? " · clique num peso para editar" : "");
    }

    function play() {
      if (idx >= steps().length - 1) go(0);
      node.querySelector("[data-act=play]").innerHTML = ICON.pause;
      timer = setInterval(() => go(idx + 1), 850 / speed);
    }
    function stop() {
      if (timer) clearInterval(timer);
      timer = null;
      const b = node.querySelector("[data-act=play]");
      if (b) b.innerHTML = ICON.play;
    }
    function toggle() { timer ? stop() : play(); }

    // init
    view.setGraph(o.graph, o.start);
    setAlg(alg);
    updateHint();

    return { stop, play, toggle, view, setAlg, next: () => go(idx + 1), prev: () => go(idx - 1), replay() { stop(); go(0); setTimeout(play, 500); } };
  }

  window.AlgoWidget = AlgoWidget;
  window.ICON = ICON;
  window.CODE_LINES = CODE_LINES;
})();
