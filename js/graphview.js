/* Desenho e edicao de um grafo pequeno em SVG (D3).
 *
 * Mantem o modelo { nodes: [{id, x, y}], edges: [{id, u, v, w}] } com
 * vertices numerados 1..n, como na biblioteca em C++.
 */
(function () {
  const W = 800, H = 520, R = 19;
  let edgeSeq = 1000;

  function clone(graph) {
    return {
      nodes: graph.nodes.map((n) => ({ ...n })),
      edges: graph.edges.map((e) => ({ ...e, id: e.id ?? edgeSeq++ })),
    };
  }

  class GraphView {
    constructor(container, opts = {}) {
      this.el = container;
      this.opts = Object.assign({ showWeights: true, editable: false, onChange: null, onOrigin: null, onHover: null }, opts);
      this.mode = "move";
      this.pendingEdge = null;
      this.start = 1;
      this.state = null;
      this.hoverVertex = null;

      this.svg = d3.select(container).append("svg")
        .attr("viewBox", `0 0 ${W} ${H}`)
        .attr("preserveAspectRatio", "xMidYMid meet");
      this.gEdges = this.svg.append("g");
      this.gNodes = this.svg.append("g");

      this.svg.on("click", (ev) => {
        if (ev.target !== this.svg.node()) return;
        if (this.mode === "add-node" && this.opts.editable) {
          const [x, y] = d3.pointer(ev, this.svg.node());
          this.graph.nodes.push({ id: this.graph.nodes.length + 1, x, y });
          this.changed();
        } else if (this.pendingEdge) {
          this.pendingEdge = null;
          this.draw();
        }
      });
    }

    setGraph(graph, start = 1) {
      this.graph = clone(graph);
      this.start = Math.min(Math.max(1, start), Math.max(1, this.graph.nodes.length));
      this.pendingEdge = null;
      this.draw();
    }

    setMode(mode) {
      this.mode = mode;
      this.pendingEdge = null;
      this.el.classList.toggle("mode-add-node", mode === "add-node");
      this.draw();
    }

    changed() {
      this.draw();
      this.opts.onChange && this.opts.onChange(this.graph);
    }

    // ------------------------------------------------------------ edicao
    removeNode(id) {
      const g = this.graph;
      g.nodes = g.nodes.filter((n) => n.id !== id);
      g.edges = g.edges.filter((e) => e.u !== id && e.v !== id);
      const map = new Map();
      g.nodes.forEach((n, i) => { map.set(n.id, i + 1); n.id = i + 1; });
      g.edges.forEach((e) => { e.u = map.get(e.u); e.v = map.get(e.v); });
      if (this.start === id) this.start = 1;
      else if (this.start > id) this.start -= 1;
      this.opts.onOrigin && this.opts.onOrigin(this.start);
      this.changed();
    }

    nodeClick(d) {
      if (!this.opts.editable && this.mode !== "move") return;
      if (this.mode === "move") {
        if (!this.opts.onOrigin) return;
        this.start = d.id;
        this.opts.onOrigin(d.id);
        this.draw();
      } else if (this.mode === "add-edge") {
        if (!this.pendingEdge) { this.pendingEdge = d.id; this.draw(); return; }
        if (this.pendingEdge !== d.id) {
          const u = this.pendingEdge, v = d.id;
          const w = Math.round((1 + Math.random() * 9) * 10) / 10;
          this.graph.edges.push({ id: edgeSeq++, u, v, w: this.opts.showWeights ? w : 1 });
          this.pendingEdge = null;
          this.changed();
        }
      } else if (this.mode === "delete") {
        this.removeNode(d.id);
      }
    }

    edgeClick(d) {
      if (this.opts.editable && this.mode === "delete") {
        this.graph.edges = this.graph.edges.filter((e) => e.id !== d.id);
        this.changed();
      }
    }

    editWeight(d, labelNode) {
      if (!this.opts.editable || !this.opts.showWeights || this.mode === "delete") return;
      const box = labelNode.getBoundingClientRect();
      const host = this.el.getBoundingClientRect();
      const input = document.createElement("input");
      input.className = "w-input";
      input.value = d.w;
      input.style.left = `${box.left - host.left + box.width / 2 - 35}px`;
      input.style.top = `${box.top - host.top + box.height / 2 - 15}px`;
      this.el.appendChild(input);
      input.focus();
      input.select();
      let done = false;
      const commit = (save) => {
        if (done) return;
        done = true;
        const v = parseFloat(String(input.value).replace(",", "."));
        input.remove();
        if (save && Number.isFinite(v) && v !== d.w) {
          d.w = v;
          this.changed();
        }
      };
      input.addEventListener("keydown", (ev) => {
        ev.stopPropagation();
        if (ev.key === "Enter") commit(true);
        if (ev.key === "Escape") commit(false);
      });
      input.addEventListener("blur", () => commit(true));
    }

    // ------------------------------------------------------------ desenho
    draw() {
      const g = this.graph;
      if (!g) return;
      const byId = new Map(g.nodes.map((n) => [n.id, n]));
      const self = this;

      const edges = this.gEdges.selectAll("g.g-edge").data(g.edges, (d) => d.id);
      edges.exit().remove();
      const eIn = edges.enter().append("g").attr("class", "g-edge");
      eIn.append("line").attr("class", "hit");
      eIn.append("line").attr("class", "vis");
      const lbl = eIn.append("g").attr("class", "wlabel");
      lbl.append("rect").attr("class", "wbg");
      lbl.append("text").attr("class", "wt");
      eIn.on("click", function (ev, d) { ev.stopPropagation(); self.edgeClick(d); });
      eIn.select(".wlabel").on("click", function (ev, d) { ev.stopPropagation(); self.edgeClick(d); self.editWeight(d, this); });

      const eAll = eIn.merge(edges);
      eAll.selectAll("line")
        .attr("x1", (d) => byId.get(d.u).x).attr("y1", (d) => byId.get(d.u).y)
        .attr("x2", (d) => byId.get(d.v).x).attr("y2", (d) => byId.get(d.v).y);
      eAll.select(".wlabel")
        .style("display", this.opts.showWeights ? null : "none")
        .attr("transform", (d) => {
          const a = byId.get(d.u), b = byId.get(d.v);
          return `translate(${(a.x + b.x) / 2},${(a.y + b.y) / 2})`;
        });
      eAll.select(".wt").text((d) => Engine.fmt(d.w));
      eAll.select(".wbg").each(function (d) {
        const w = Math.max(26, String(Engine.fmt(d.w)).length * 7.6 + 12);
        d3.select(this).attr("x", -w / 2).attr("y", -11).attr("width", w).attr("height", 22);
      });

      const nodes = this.gNodes.selectAll("g.g-node").data(g.nodes, (d) => d.id);
      nodes.exit().remove();
      const nIn = nodes.enter().append("g").attr("class", "g-node");
      nIn.append("circle").attr("class", "glow").attr("r", R + 4);
      nIn.append("circle").attr("class", "ring").attr("r", R + 7);
      nIn.append("circle").attr("class", "body").attr("r", R);
      nIn.append("text").attr("class", "lbl");
      nIn.append("text").attr("class", "dist").attr("y", -R - 9);
      nIn.append("text").attr("class", "ord").attr("x", R - 2).attr("y", R + 10);
      nIn.on("mouseenter", (ev, d) => { this.hoverVertex = d.id; this.opts.onHover && this.opts.onHover(d.id); })
        .on("mouseleave", () => { this.hoverVertex = null; this.opts.onHover && this.opts.onHover(null); });

      const drag = d3.drag()
        .clickDistance(4)
        .filter(() => this.mode === "move")
        .on("drag", function (ev, d) {
          d.x = Math.max(R, Math.min(W - R, ev.x));
          d.y = Math.max(R + 14, Math.min(H - R, ev.y));
          self.draw();
        });
      nIn.call(drag);
      nIn.on("click", function (ev, d) { ev.stopPropagation(); self.nodeClick(d); });

      const nAll = nIn.merge(nodes);
      nAll.attr("transform", (d) => `translate(${d.x},${d.y})`);
      nAll.select(".lbl").text((d) => d.id);
      nAll.classed("pending-edge", (d) => d.id === this.pendingEdge);

      this.render(this.state);
    }

    /* Aplica o estado de um passo do algoritmo (ou null = grafo limpo). */
    render(state, extra = {}) {
      this.state = state;
      const s = state;
      const g = this.graph;
      if (!g) return;
      const visited = new Set(s ? s.order : []);
      const negSet = new Set(extra.negEdges || []);
      const pathEdges = new Set(extra.pathEdges || []);
      const pathNodes = new Set(extra.pathNodes || []);
      const highlightNodes = extra.highlightNodes ? new Set(extra.highlightNodes) : null;
      const highlightEdges = extra.highlightEdges ? new Set(extra.highlightEdges) : null;

      const treeEdge = new Map();
      if (s) {
        for (let v = 1; v < s.parEdge.length; v++) {
          if (s.parEdge[v] != null) treeEdge.set(s.parEdge[v], visited.has(v) ? "tree" : "tentative");
        }
      }

      this.gEdges.selectAll("g.g-edge")
        .classed("tree", (d) => treeEdge.get(d.id) === "tree")
        .classed("tentative", (d) => treeEdge.get(d.id) === "tentative")
        .classed("active", (d) => !!s && s.edge === d.id)
        .classed("neg", (d) => negSet.has(d.id))
        .classed("path", (d) => pathEdges.has(d.id))
        .classed("hover", (d) => !!highlightEdges && highlightEdges.has(d.id))
        .classed("faded", (d) => (pathEdges.size > 0 && !pathEdges.has(d.id)) || (!!highlightEdges && !highlightEdges.has(d.id)));

      const showDist = !!s;
      this.gNodes.selectAll("g.g-node")
        .classed("origin", (d) => d.id === this.start)
        .classed("visited", (d) => visited.has(d.id))
        .classed("frontier", (d) => !!s && s.frontierVerts.has(d.id) && !visited.has(d.id))
        .classed("current", (d) => !!s && s.current === d.id)
        .classed("target", (d) => !!s && s.target === d.id && s.current !== d.id)
        .classed("onpath", (d) => pathNodes.has(d.id))
        .classed("hover", (d) => !!highlightNodes && highlightNodes.has(d.id))
        .classed("faded", (d) => (pathNodes.size > 0 && !pathNodes.has(d.id)) || (!!highlightNodes && highlightNodes.size > 1 && !highlightNodes.has(d.id)))
        .each(function (d) {
          const sel = d3.select(this);
          sel.select(".dist").text(showDist && s.dist[d.id] !== Infinity ? Engine.fmt(s.dist[d.id]) : showDist ? "∞" : "");
          const k = s ? s.order.indexOf(d.id) : -1;
          sel.select(".ord").text(k >= 0 ? `#${k + 1}` : "");
        });
    }
  }

  // --------------------------------------------------------- geradores
  function randomGraph(n, extraEdges, weighted, seed) {
    let t = seed || Math.floor(Math.random() * 1e9);
    const rnd = () => { t = (t * 1664525 + 1013904223) % 4294967296; return t / 4294967296; };
    const nodes = d3.range(1, n + 1).map((id) => ({ id, x: 80 + rnd() * (W - 160), y: 70 + rnd() * (H - 140) }));
    const edges = [];
    const has = new Set();
    const add = (u, v) => {
      const k = u < v ? `${u}-${v}` : `${v}-${u}`;
      if (u === v || has.has(k)) return;
      has.add(k);
      edges.push({ id: edgeSeq++, u, v, w: weighted ? Math.round((1 + rnd() * 9) * 10) / 10 : 1 });
    };
    // arvore aleatoria (conexo) + arestas extras entre vizinhos proximos
    for (let v = 2; v <= n; v++) add(v, 1 + Math.floor(rnd() * (v - 1)));
    let tries = 0;
    while (edges.length < n - 1 + extraEdges && tries++ < 500) {
      const u = 1 + Math.floor(rnd() * n), v = 1 + Math.floor(rnd() * n);
      const a = nodes[u - 1], b = nodes[v - 1];
      if (Math.hypot(a.x - b.x, a.y - b.y) < 330) add(u, v);
    }
    // espalha com forcas para nao embolar
    const sim = d3.forceSimulation(nodes)
      .force("charge", d3.forceManyBody().strength(-520))
      .force("link", d3.forceLink(edges.map((e) => ({ source: e.u - 1, target: e.v - 1 }))).distance(95))
      .force("center", d3.forceCenter(W / 2, H / 2))
      .force("collide", d3.forceCollide(R * 2.4))
      .stop();
    for (let i = 0; i < 320; i++) sim.tick();
    fitInside(nodes);
    return { nodes: nodes.map(({ id, x, y }) => ({ id, x, y })), edges };
  }

  function fitInside(nodes) {
    const xs = nodes.map((n) => n.x), ys = nodes.map((n) => n.y);
    const [x0, x1] = d3.extent(xs), [y0, y1] = d3.extent(ys);
    const pad = 60;
    const sx = (W - 2 * pad) / Math.max(1, x1 - x0);
    const sy = (H - 2 * pad - 10) / Math.max(1, y1 - y0);
    const s = Math.min(sx, sy, 1.6);
    const cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
    nodes.forEach((n) => { n.x = W / 2 + (n.x - cx) * s; n.y = H / 2 + 8 + (n.y - cy) * s; });
  }

  // Exemplos fixos
  const EXEMPLOS = {
    // Figura 1 do enunciado (com o peso negativo -9.5)
    pdf: {
      nodes: [
        { id: 1, x: 190, y: 220 }, { id: 2, x: 360, y: 110 }, { id: 3, x: 600, y: 170 },
        { id: 4, x: 620, y: 380 }, { id: 5, x: 380, y: 410 },
      ],
      edges: [
        { u: 1, v: 2, w: 0.1 }, { u: 2, v: 5, w: 0.2 }, { u: 5, v: 3, w: 5 },
        { u: 3, v: 4, w: -9.5 }, { u: 4, v: 5, w: 2.3 }, { u: 1, v: 5, w: 1 },
      ],
    },
    // Grafo para BFS/DFS: dois "andares" e um ciclo, bom para comparar ordens
    busca: {
      nodes: [
        { id: 1, x: 400, y: 70 }, { id: 2, x: 230, y: 170 }, { id: 3, x: 400, y: 190 }, { id: 4, x: 570, y: 170 },
        { id: 5, x: 130, y: 300 }, { id: 6, x: 290, y: 310 }, { id: 7, x: 480, y: 320 }, { id: 8, x: 650, y: 300 },
        { id: 9, x: 200, y: 440 }, { id: 10, x: 390, y: 450 }, { id: 11, x: 590, y: 440 },
      ],
      edges: [
        [1, 2], [1, 3], [1, 4], [2, 5], [2, 6], [3, 6], [3, 7], [4, 7], [4, 8],
        [5, 9], [6, 9], [6, 10], [7, 10], [7, 11], [8, 11],
      ].map(([u, v]) => ({ u, v, w: 1 })),
    },
    // Grafo ponderado em que o caminho com menos arestas NAO e o mais curto
    dijkstra: {
      nodes: [
        { id: 1, x: 110, y: 260 }, { id: 2, x: 270, y: 120 }, { id: 3, x: 270, y: 400 }, { id: 4, x: 450, y: 90 },
        { id: 5, x: 440, y: 260 }, { id: 6, x: 450, y: 430 }, { id: 7, x: 630, y: 160 }, { id: 8, x: 640, y: 370 },
        { id: 9, x: 740, y: 265 },
      ],
      edges: [
        { u: 1, v: 2, w: 4 }, { u: 1, v: 3, w: 1.5 }, { u: 1, v: 5, w: 9 }, { u: 2, v: 4, w: 2 }, { u: 2, v: 5, w: 3 },
        { u: 3, v: 5, w: 2 }, { u: 3, v: 6, w: 6 }, { u: 4, v: 7, w: 2.5 }, { u: 5, v: 7, w: 5 }, { u: 5, v: 8, w: 1.2 },
        { u: 6, v: 8, w: 1 }, { u: 7, v: 9, w: 1.5 }, { u: 8, v: 9, w: 4 }, { u: 4, v: 5, w: 1 },
      ],
    },
  };
  for (const g of Object.values(EXEMPLOS)) g.edges.forEach((e) => { e.id = edgeSeq++; });

  function gridGraph(r, c) {
    const nodes = [], edges = [];
    for (let i = 0; i < r; i++) for (let j = 0; j < c; j++) {
      nodes.push({ id: i * c + j + 1, x: W / 2 + (j - (c - 1) / 2) * 120, y: H / 2 + 8 + (i - (r - 1) / 2) * 105 });
      if (j) edges.push({ id: edgeSeq++, u: i * c + j, v: i * c + j + 1, w: 1 });
      if (i) edges.push({ id: edgeSeq++, u: (i - 1) * c + j + 1, v: i * c + j + 1, w: 1 });
    }
    return { nodes, edges };
  }
  function treeGraph(levels) {
    const nodes = [], edges = [];
    const total = 2 ** levels - 1;
    for (let id = 1; id <= total; id++) {
      const d = Math.floor(Math.log2(id));
      const k = id - 2 ** d;
      nodes.push({ id, x: 40 + ((k + 0.5) / 2 ** d) * (W - 80), y: 60 + d * ((H - 110) / (levels - 1)) });
      if (id > 1) edges.push({ id: edgeSeq++, u: id >> 1, v: id, w: 1 });
    }
    return { nodes, edges };
  }
  function cycleGraph(n) {
    const nodes = d3.range(1, n + 1).map((id) => {
      const a = ((id - 1) / n) * 2 * Math.PI - Math.PI / 2;
      return { id, x: W / 2 + Math.cos(a) * 200, y: H / 2 + 8 + Math.sin(a) * 200 };
    });
    const edges = nodes.map((nd) => ({ id: edgeSeq++, u: nd.id, v: (nd.id % n) + 1, w: 1 }));
    edges.push({ id: edgeSeq++, u: 1, v: Math.floor(n / 2) + 1, w: 1 });
    return { nodes, edges };
  }

  window.GraphView = GraphView;
  window.GraphGen = { randomGraph, gridGraph, treeGraph, cycleGraph, EXEMPLOS, clone, W, H };
})();
