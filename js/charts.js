/* Visualizacoes dos dados reais (window.DADOS) e demonstracoes auxiliares. */
(function () {
  const pt = d3.formatLocale({ decimal: ",", thousands: ".", grouping: [3], currency: ["R$", ""] });
  const fInt = pt.format(",d");
  const fNum = (x, p = 1) => pt.format(`,.${p}f`)(x);
  const fMs = (ms) => (ms >= 1000 ? `${fNum(ms / 1000, 2)} s` : `${fNum(ms, ms < 10 ? 2 : 1)} ms`);
  const fBig = (x) => {
    const t = (v) => fNum(v, 1).replace(/,0$/, "");
    if (x >= 1e9) return `${t(x / 1e9)} bi`;
    if (x >= 1e6) return `${t(x / 1e6)} mi`;
    if (x >= 1e3) return `${t(x / 1e3)} mil`;
    return fInt(x);
  };
  const fBytes = (b) => {
    const u = ["B", "KB", "MB", "GB", "TB", "PB", "EB"];
    let i = 0;
    while (b >= 1024 && i < u.length - 1) { b /= 1024; i++; }
    return `${fNum(b, b < 10 ? 2 : b < 100 ? 1 : 0)} ${u[i]}`;
  };
  const fDur = (s) => {
    if (s < 1e-3) return `${fNum(s * 1e6, 0)} µs`;
    if (s < 1) return `${fNum(s * 1e3, 0)} ms`;
    if (s < 120) return `${fNum(s, 1)} s`;
    if (s < 7200) return `${fNum(s / 60, 0)} min`;
    if (s < 172800) return `${fNum(s / 3600, 1)} h`;
    return `${fNum(s / 86400, 0)} dias`;
  };

  const css = (v) => getComputedStyle(document.documentElement).getPropertyValue(v).trim();
  const SERIES = [
    { key: "BFS", nome: "BFS", cor: () => css("--bfs") },
    { key: "DFS_Iterativa", nome: "DFS iterativa", cor: () => css("--dfs") },
    { key: "Dijkstra_Heap", nome: "Dijkstra (heap)", cor: () => css("--dij") },
  ];

  // ------------------------------------------------------------ tooltip
  const tip = d3.select("body").append("div").attr("class", "tooltip");
  function showTip(ev, html) {
    tip.html(html).style("opacity", 1);
    const r = tip.node().getBoundingClientRect();
    let x = ev.clientX + 14, y = ev.clientY + 14;
    if (x + r.width > innerWidth - 8) x = ev.clientX - r.width - 14;
    if (y + r.height > innerHeight - 8) y = ev.clientY - r.height - 14;
    tip.style("left", `${x}px`).style("top", `${y}px`);
  }
  const hideTip = () => tip.style("opacity", 0);

  function sized(container, draw) {
    const ro = new ResizeObserver(() => {
      const { width, height } = container.getBoundingClientRect();
      if (width > 10 && height > 10) draw(width, height);
    });
    ro.observe(container);
    return ro;
  }

  // ------------------------------------------- desempenho (log x log)
  function PerfChart(container, { onSelect, selected }) {
    const grafos = DADOS.grafos;
    let stat = "media";
    const on = new Set(SERIES.map((s) => s.key));
    let sel = selected;

    const svg = d3.select(container).append("svg");
    let size = [600, 400];

    function draw(w, h) {
      size = [w, h];
      svg.selectAll("*").remove();
      const m = { t: 14, r: 18, b: 46, l: 84 };
      const x = d3.scaleLog().domain([d3.min(grafos, (g) => g.v_mais_e) / 1.4, d3.max(grafos, (g) => g.v_mais_e) * 1.4]).range([m.l, w - m.r]);
      const all = grafos.flatMap((g) => SERIES.map((s) => g.benchmark[s.key][stat]));
      const y = d3.scaleLog().domain([d3.min(all) / 1.6, d3.max(all) * 1.6]).range([h - m.b, m.t]).nice();

      svg.append("g").attr("class", "gridline").attr("transform", `translate(${m.l},0)`)
        .call(d3.axisLeft(y).ticks(5).tickSize(-(w - m.l - m.r)).tickFormat(""));
      svg.append("g").attr("class", "axis").attr("transform", `translate(0,${h - m.b})`)
        .call(d3.axisBottom(x).ticks(5, (d) => fBig(d)));
      svg.append("g").attr("class", "axis").attr("transform", `translate(${m.l},0)`)
        .call(d3.axisLeft(y).ticks(5, (d) => fMs(d)));
      svg.append("text").attr("class", "axis-label").attr("x", (m.l + w - m.r) / 2).attr("y", h - 8).attr("text-anchor", "middle").text("tamanho do grafo: V + E (escala log)");
      svg.append("text").attr("class", "axis-label").attr("transform", `translate(12,${(m.t + h - m.b) / 2}) rotate(-90)`).attr("text-anchor", "middle").text("tempo (escala log)");

      // faixa do grafo selecionado
      const sg = grafos.find((g) => g.id === sel);
      if (sg) {
        svg.append("line").attr("x1", x(sg.v_mais_e)).attr("x2", x(sg.v_mais_e)).attr("y1", m.t).attr("y2", h - m.b)
          .attr("stroke", css("--line")).attr("stroke-width", 14).attr("stroke-opacity", .55);
      }

      const sorted = grafos.slice().sort((a, b) => a.v_mais_e - b.v_mais_e);
      for (const s of SERIES) {
        if (!on.has(s.key)) continue;
        const pts = sorted.map((g) => ({ g, x: x(g.v_mais_e), y: y(g.benchmark[s.key][stat]) }));
        const path = svg.append("path").datum(pts).attr("fill", "none").attr("stroke", s.cor()).attr("stroke-width", 2.2)
          .attr("stroke-opacity", .9).attr("d", d3.line().x((d) => d.x).y((d) => d.y).curve(d3.curveMonotoneX));
        const len = path.node().getTotalLength();
        path.attr("stroke-dasharray", `${len} ${len}`).attr("stroke-dashoffset", len)
          .transition().duration(900).ease(d3.easeCubicOut).attr("stroke-dashoffset", 0);
        svg.append("g").selectAll("circle").data(pts).join("circle")
          .attr("cx", (d) => d.x).attr("cy", (d) => d.y).attr("r", (d) => (d.g.id === sel ? 6.5 : 4.5))
          .attr("fill", (d) => (d.g.ponderado ? s.cor() : css("--bg"))).attr("stroke", s.cor()).attr("stroke-width", 2)
          .style("cursor", "pointer")
          .on("mousemove", (ev, d) => showTip(ev, `<b>${d.g.nome}</b>${d.g.ponderado ? '<span class="badge w">pesos</span>' : ""}<br>
            <span class="muted">n</span> ${fInt(d.g.n)} · <span class="muted">m</span> ${fInt(d.g.m)}<br>
            <span style="color:${s.cor()}">${s.nome}</span>: <b>${fMs(d.g.benchmark[s.key][stat])}</b> (${stat})<br>
            <span class="muted">clique para ver as 100 execuções</span>`))
          .on("mouseleave", hideTip)
          .on("click", (ev, d) => { sel = d.g.id; draw(...size); onSelect && onSelect(d.g.id); });
      }
    }
    sized(container, draw);
    return {
      setStat(s) { stat = s; draw(...size); },
      toggle(k) { on.has(k) ? on.delete(k) : on.add(k); draw(...size); },
      select(id) { sel = id; draw(...size); },
    };
  }

  // ------------------------- barras: quanto cada busca leva, por grafo
  function PerfBars(container, { onSelect, selected }) {
    const svg = d3.select(container).append("svg");
    let sel = selected, size = [600, 400], stat = "media";
    function draw(w, h, animate = true) {
      size = [w, h];
      svg.selectAll("*").remove();
      const grafos = DADOS.grafos.slice().sort((a, b) => a.v_mais_e - b.v_mais_e);
      const m = { t: 8, r: 92, b: 34, l: 150 };
      const y0 = d3.scaleBand().domain(grafos.map((g) => g.id)).range([m.t, h - m.b]).padding(0.22);
      const y1 = d3.scaleBand().domain(SERIES.map((s) => s.key)).range([0, y0.bandwidth()]).padding(0.12);
      const all = grafos.flatMap((g) => SERIES.map((s) => g.benchmark[s.key][stat]));
      const x = d3.scaleLog().domain([d3.min(all) / 1.5, d3.max(all) * 1.3]).range([m.l, w - m.r]);
      svg.append("g").attr("class", "gridline").attr("transform", `translate(0,${h - m.b})`)
        .call(d3.axisBottom(x).ticks(5).tickSize(-(h - m.t - m.b)).tickFormat(""));
      svg.append("g").attr("class", "axis").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).ticks(5, (d) => fMs(d)));
      const row = svg.append("g").selectAll("g").data(grafos).join("g")
        .attr("transform", (g) => `translate(0,${y0(g.id)})`).style("cursor", "pointer")
        .on("click", (ev, g) => { sel = g.id; draw(...size, false); onSelect && onSelect(g.id); });
      row.append("rect").attr("x", 0).attr("width", w).attr("y", -3).attr("height", y0.bandwidth() + 6).attr("rx", 8)
        .attr("fill", (g) => (g.id === sel ? "rgba(122,162,255,.10)" : "transparent"));
      row.append("text").attr("x", 10).attr("y", y0.bandwidth() / 2).attr("dy", "0.35em").attr("fill", (g) => (g.id === sel ? "#fff" : css("--text-2")))
        .attr("font-size", 13).text((g) => g.nome);
      row.append("text").attr("x", 10).attr("y", y0.bandwidth() / 2 + 14).attr("dy", "0.35em").attr("fill", css("--faint"))
        .attr("font-size", 10.5).attr("font-family", "var(--font-mono)").text((g) => `V+E ${fBig(g.v_mais_e)}${g.ponderado ? " · pesos" : ""}`);
      for (const s of SERIES) {
        row.append("rect").attr("x", m.l).attr("y", y1(s.key)).attr("height", y1.bandwidth()).attr("rx", 3).attr("fill", s.cor())
          .attr("width", animate ? 0 : (g) => x(g.benchmark[s.key][stat]) - m.l)
          .on("mousemove", (ev, g) => showTip(ev, `<b>${g.nome}</b><br><span style="color:${s.cor()}">${s.nome}</span>: <b>${fMs(g.benchmark[s.key][stat])}</b>`))
          .on("mouseleave", hideTip)
          .transition().duration(animate ? 900 : 0).delay((g, i) => (animate ? i * 60 : 0))
          .attr("width", (g) => x(g.benchmark[s.key][stat]) - m.l);
      }
      row.append("text").attr("x", w - m.r + 10).attr("y", y0.bandwidth() / 2).attr("dy", "0.35em")
        .attr("font-family", "var(--font-display)").attr("font-size", 16).attr("font-weight", 600)
        .attr("fill", (g) => { const r = g.benchmark.Dijkstra_Heap[stat] / g.benchmark.BFS[stat]; return r > 1.3 ? css("--warn") : css("--muted"); })
        .text((g) => `${fNum(g.benchmark.Dijkstra_Heap[stat] / g.benchmark.BFS[stat], 1)}×`);
      svg.append("text").attr("x", w - m.r + 10).attr("y", h - 10).attr("fill", css("--faint")).attr("font-size", 10.5)
        .attr("font-family", "var(--font-mono)").text("Dijkstra÷BFS");
    }
    sized(container, (w, h) => draw(w, h));
    return { setStat(s) { stat = s; draw(...size); }, select(id) { sel = id; draw(...size, false); } };
  }

  // ------------------------------------ distribuicao das 100 execucoes
  function StripChart(container) {
    const svg = d3.select(container).append("svg");
    let gid = null, size = [400, 260];
    function draw(w, h) {
      size = [w, h];
      svg.selectAll("*").remove();
      const g = DADOS.grafos.find((x) => x.id === gid);
      if (!g) return;
      const m = { t: 10, r: 18, b: 34, l: 108 };
      const vals = SERIES.flatMap((s) => g.benchmark[s.key].execucoes);
      const x = d3.scaleLinear().domain(d3.extent(vals)).nice().range([m.l, w - m.r]);
      const y = d3.scaleBand().domain(SERIES.map((s) => s.key)).range([m.t, h - m.b]).padding(.28);
      svg.append("g").attr("class", "gridline").attr("transform", `translate(0,${h - m.b})`)
        .call(d3.axisBottom(x).ticks(5).tickSize(-(h - m.t - m.b)).tickFormat(""));
      svg.append("g").attr("class", "axis").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).ticks(5).tickFormat((d) => fMs(d)));
      svg.append("g").attr("class", "axis").attr("transform", `translate(${m.l},0)`).call(d3.axisLeft(y).tickFormat((k) => SERIES.find((s) => s.key === k).nome).tickSize(0)).select(".domain").remove();

      for (const s of SERIES) {
        const arr = g.benchmark[s.key].execucoes.slice().sort(d3.ascending);
        const q1 = d3.quantile(arr, .25), q3 = d3.quantile(arr, .75), med = d3.quantile(arr, .5);
        const y0 = y(s.key), bh = y.bandwidth();
        svg.append("rect").attr("x", x(q1)).attr("width", Math.max(2, x(q3) - x(q1))).attr("y", y0).attr("height", bh)
          .attr("rx", 6).attr("fill", s.cor()).attr("fill-opacity", .12).attr("stroke", s.cor()).attr("stroke-opacity", .4);
        svg.append("line").attr("x1", x(med)).attr("x2", x(med)).attr("y1", y0 - 3).attr("y2", y0 + bh + 3).attr("stroke", s.cor()).attr("stroke-width", 2.5);
        let seed = 7;
        const rnd = () => ((seed = (seed * 16807) % 2147483647) / 2147483647);
        svg.append("g").selectAll("circle").data(g.benchmark[s.key].execucoes).join("circle")
          .attr("cx", (d) => x(d)).attr("cy", () => y0 + bh / 2 + (rnd() - .5) * bh * .8).attr("r", 0)
          .attr("fill", s.cor()).attr("fill-opacity", .75)
          .on("mousemove", (ev, d) => showTip(ev, `<span style="color:${s.cor()}">${s.nome}</span>: <b>${fMs(d)}</b>`))
          .on("mouseleave", hideTip)
          .transition().delay((d, i) => i * 4).duration(300).attr("r", 2.6);
      }
    }
    sized(container, draw);
    return { show(id) { gid = id; draw(...size); } };
  }

  // ------------------------------------------------- histograma de graus
  function DegreeChart(container) {
    const svg = d3.select(container).append("svg");
    let gid = null, logY = false, size = [500, 300], destaques = [];
    function draw(w, h) {
      size = [w, h];
      svg.selectAll("*").remove();
      const data = DADOS.graus[gid];
      const g = DADOS.grafos.find((x) => x.id === gid);
      if (!data || !g) return;
      const m = { t: 16, r: 16, b: 42, l: 66 };
      const x = d3.scaleLinear().domain([d3.min(data, (d) => d[0]), d3.max(data, (d) => d[0]) + 1]).range([m.l, w - m.r]);
      const maxF = d3.max(data, (d) => d[1]);
      const y = logY ? d3.scaleLog().domain([0.8, maxF * 1.3]).range([h - m.b, m.t]) : d3.scaleLinear().domain([0, maxF * 1.08]).range([h - m.b, m.t]);
      svg.append("g").attr("class", "gridline").attr("transform", `translate(${m.l},0)`).call(d3.axisLeft(y).ticks(5).tickSize(-(w - m.l - m.r)).tickFormat(""));
      svg.append("g").attr("class", "axis").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).ticks(8).tickFormat(d3.format("d")));
      const pow10 = (d) => Math.abs(Math.log10(d) - Math.round(Math.log10(d))) < 1e-9;
      svg.append("g").attr("class", "axis").attr("transform", `translate(${m.l},0)`)
        .call(d3.axisLeft(y).ticks(5).tickFormat((d) => (logY && !pow10(d) ? "" : fBig(d))));
      svg.append("text").attr("class", "axis-label").attr("x", (m.l + w - m.r) / 2).attr("y", h - 6).attr("text-anchor", "middle").text("grau");
      const bw = Math.max(1, x(1) - x(0) - 1);
      const col = g.ponderado ? css("--vec") : css("--dij");
      svg.append("g").selectAll("rect").data(data).join("rect")
        .attr("x", (d) => x(d[0])).attr("width", bw).attr("y", h - m.b).attr("height", 0).attr("rx", Math.min(3, bw / 2))
        .attr("fill", col).attr("fill-opacity", .85)
        .on("mousemove", (ev, d) => showTip(ev, `grau <b>${d[0]}</b>: ${fInt(d[1])} vértices (${fNum((100 * d[1]) / g.n, 2)}%)`))
        .on("mouseleave", hideTip)
        .transition().duration(600).delay((d, i) => i * 2)
        .attr("y", (d) => y(Math.max(logY ? 0.8 : 0, d[1]))).attr("height", (d) => h - m.b - y(Math.max(logY ? 0.8 : 0, d[1])));
      const mark = (v, label, c) => {
        svg.append("line").attr("x1", x(v)).attr("x2", x(v)).attr("y1", m.t).attr("y2", h - m.b).attr("stroke", c).attr("stroke-dasharray", "4 4").attr("stroke-width", 1.5);
        svg.append("text").attr("x", x(v) + 5).attr("y", m.t + 10).attr("fill", c).attr("font-size", 11).attr("font-family", "var(--font-mono)").text(label);
      };
      mark(g.graus.medio, `média ${fNum(g.graus.medio, 2)}`, css("--warn"));
      // vertices de grau muito alto: uma barra de 1 vertice some no grafico, entao ganha um rotulo
      const dc = css("--dfs");
      destaques.forEach((d, i) => {
        const cx = x(d.grau) + bw / 2, base = y(logY ? 1 : 0), top = m.t + 40 + i * 34;
        svg.append("line").attr("x1", cx).attr("x2", cx).attr("y1", top + 6).attr("y2", base - 8).attr("stroke", dc).attr("stroke-width", 1.5).attr("stroke-dasharray", "2 3");
        svg.append("circle").attr("cx", cx).attr("cy", base - 4).attr("r", 6).attr("fill", dc).attr("class", "deg-hub");
        svg.append("text").attr("x", cx - 8).attr("y", top).attr("text-anchor", "end").attr("fill", dc).attr("font-size", 12.5).attr("font-weight", 600).text(d.nome);
        svg.append("text").attr("x", cx - 8).attr("y", top + 14).attr("text-anchor", "end").attr("fill", css("--muted")).attr("font-size", 11).attr("font-family", "var(--font-mono)").text(`grau ${d.grau}`);
      });
    }
    sized(container, draw);
    return {
      show(id, marcar = []) { gid = id; destaques = marcar; draw(...size); },
      setLog(v) { logY = v; draw(...size); },
    };
  }

  // ------------------------------------------------ amostra das arvores
  function TreeChart(container, onInfo) {
    const svg = d3.select(container).append("svg").attr("class", "tree-svg");
    let gid = null, alg = "bfs", size = [500, 500], anim = null;
    const ALGCOL = { bfs: "--bfs", dfs: "--dfs", dijkstra: "--dij" };

    function build() {
      const rows = DADOS.arvores[gid][alg];
      const byV = new Map(rows.map((r) => [r[0], r]));
      return d3.stratify()
        .id((r) => r[0])
        .parentId((r) => (r[1] === r[0] || !byV.has(r[1]) ? null : r[1]))(rows);
    }

    function draw(w, h, animate = false) {
      size = [w, h];
      svg.selectAll("*").remove();
      if (!gid) return;
      const root = build();
      const R = Math.min(w, h) / 2 - 20;
      d3.tree().size([2 * Math.PI, R]).separation((a, b) => (a.parent === b.parent ? 1 : 2) / Math.max(1, a.depth))(root);
      const col = css(ALGCOL[alg]);
      const g = svg.append("g").attr("transform", `translate(${w / 2},${h / 2})`);
      const maxOrd = d3.max(root.descendants(), (d) => d.data[3]);
      const op = d3.scaleLinear().domain([1, maxOrd]).range([1, 0.35]);
      const radial = (a, r) => [Math.cos(a - Math.PI / 2) * r, Math.sin(a - Math.PI / 2) * r];

      const links = g.append("g").selectAll("path").data(root.links()).join("path")
        .attr("class", "link").attr("stroke", col)
        .attr("d", d3.linkRadial().angle((d) => d.x).radius((d) => d.y));
      const nodes = g.append("g").selectAll("g").data(root.descendants()).join("g")
        .attr("class", (d) => `node ${d.depth === 0 ? "root" : ""}`)
        .attr("transform", (d) => `translate(${radial(d.x, d.y)})`);
      nodes.append("circle").attr("r", (d) => (d.depth === 0 ? 7 : d.children ? 3.6 : 2.6))
        .attr("fill", col).attr("fill-opacity", (d) => op(d.data[3]));
      nodes.on("mousemove", (ev, d) => showTip(ev, `vértice <b>${fInt(d.data[0])}</b><br>pai ${fInt(d.data[1])} · ${alg === "dijkstra" ? "dist" : "nível"} <b>${fNum(d.data[2], alg === "dijkstra" ? 3 : 0)}</b><br>ordem de visita #${d.data[3]}`))
        .on("mouseleave", hideTip);

      const desc = root.descendants();
      onInfo && onInfo({
        profundidade: d3.max(desc, (d) => d.depth),
        folhas: desc.filter((d) => !d.children).length,
        filhosRaiz: root.children ? root.children.length : 0,
        total: desc.length,
      });

      if (animate) {
        nodes.style("opacity", 0);
        links.style("opacity", 0);
        nodes.transition().delay((d) => d.data[3] * 14).duration(200).style("opacity", 1);
        links.transition().delay((d) => d.target.data[3] * 14).duration(200).style("opacity", 1);
      }
    }
    sized(container, (w, h) => draw(w, h));
    return {
      show(id, a, animate = true) { gid = id; alg = a; draw(...size, animate); },
      replay() { draw(...size, true); },
    };
  }

  // --------------------------------------------- memoria lista x matriz
  const SIZEOF = { int: 12, double: 16, "long double": 32 };
  const PESO = { int: 4, double: 8, "long double": 16 };
  function memEstimate(n, m, T) {
    const VEC = 24; // cabecalho de um std::vector (ponteiro + tamanho + capacidade)
    return {
      // vector<vector<int>>: 4 bytes por celula
      matriz: (n + 1) * (n + 1) * 4 + (n + 1) * VEC,
      // vector<bool> / bitset: 1 bit por celula
      bits: Math.ceil(((n + 1) * (n + 1)) / 8) + (n + 1) * VEC,
      // vector<vector<Edge<T>>>: o que a biblioteca usa
      lista: 2 * m * SIZEOF[T] + (n + 1) * VEC,
      // CSR: offsets (n+1 ints) + vizinhos (2m ints) + pesos (2m T)
      csr: (n + 1) * 4 + 2 * m * (4 + PESO[T]),
    };
  }

  // ------------------------------------- teoria: n^2 x (n+m) log(n+m)
  // Uma linha por grafo: o ponto do heap, o do vetor e a barra entre eles.
  const opsVetor = (g) => g.n * g.n;
  const opsHeap = (g) => (g.n + g.m) * Math.log2(g.n + g.m);
  function TheoryChart(container) {
    const svg = d3.select(container).append("svg");
    const pts = DADOS.grafos.slice().sort((a, b) => a.n - b.n || a.m - b.m).map((g) => ({ g, vetor: opsVetor(g), heap: opsHeap(g) }));
    function draw(w, h) {
      svg.selectAll("*").remove();
      const m = { t: 8, r: 84, b: 42, l: 140 };
      const y = d3.scaleBand().domain(pts.map((p) => p.g.nome)).range([m.t, h - m.b]).padding(0.35);
      const x = d3.scaleLog().domain([1e6, 1e16]).range([m.l, w - m.r]);
      svg.append("g").attr("class", "gridline").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).ticks(10).tickSize(-(h - m.t - m.b)).tickFormat(""));
      svg.append("g").attr("class", "axis").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).ticks(10, (d) => `10^${Math.round(Math.log10(d))}`));
      svg.append("text").attr("class", "axis-label").attr("x", (m.l + w - m.r) / 2).attr("y", h - 6).attr("text-anchor", "middle").text("operações (escala log: cada linha da grade é 10× mais)");
      svg.append("g").attr("class", "axis").attr("transform", `translate(${m.l},0)`)
        .call(d3.axisLeft(y).tickSize(0))
        .select(".domain").remove();

      const vec = css("--vec"), heap = css("--heap");
      const gp = svg.append("g").selectAll("g").data(pts).join("g").attr("transform", (p) => `translate(0,${y(p.g.nome) + y.bandwidth() / 2})`);
      gp.append("line").attr("x1", (p) => x(p.heap)).attr("x2", (p) => x(p.heap)).attr("stroke", css("--line")).attr("stroke-width", 3)
        .transition().duration(800).delay((p, i) => i * 60).attr("x2", (p) => x(p.vetor));
      gp.append("circle").attr("cx", (p) => x(p.heap)).attr("r", 6).attr("fill", heap);
      gp.append("circle").attr("cx", (p) => x(p.heap)).attr("r", 6).attr("fill", vec)
        .transition().duration(800).delay((p, i) => i * 60).attr("cx", (p) => x(p.vetor));
      gp.append("text").attr("x", (p) => x(p.vetor) + 10).attr("dy", "0.35em").attr("fill", css("--muted")).attr("font-size", 11).attr("font-family", "var(--font-mono)")
        .text((p) => `${fInt(Math.round(p.vetor / p.heap))}×`);
      gp.append("rect").attr("x", m.l).attr("y", -y.step() / 2).attr("width", w - m.l - m.r).attr("height", y.step()).attr("fill", "transparent")
        .on("mousemove", (ev, p) => showTip(ev, `<b>${p.g.nome}</b> · n = ${fInt(p.g.n)}, m = ${fInt(p.g.m)}<br>
          <span class="c-vec">vetor</span> n² ≈ <b>${p.vetor.toExponential(1)}</b> → ${fDur(p.vetor * 1e-9)} a 1 ns/op<br>
          <span class="c-dij">heap</span> (n+m)·log(n+m) ≈ <b>${p.heap.toExponential(1)}</b> → ${fDur(p.heap * 1e-9)} a 1 ns/op<br>
          o heap faz ≈ <b>${fInt(Math.round(p.vetor / p.heap))}×</b> menos operações`))
        .on("mouseleave", hideTip);
    }
    sized(container, draw);
  }

  // --------------------------------- vetor x heap lado a lado (ao vivo)
  function DualRun(root) {
    root.innerHTML = `
      <div class="row">
        <button class="btn primary" data-a="play">${ICON.play}</button>
        <button class="btn" data-a="step">${ICON.next} passo</button>
        <button class="btn" data-a="end">${ICON.last} fim</button>
        <button class="btn" data-a="new">${ICON.dice} novo grafo</button>
        <span class="muted mono" style="font-size:12px">n =</span>
        <input type="range" class="nsl" min="6" max="40" value="14" style="width:130px;accent-color:var(--dij)"/>
        <span class="mono nval" style="font-size:13px">14</span>
        <span class="spacer"></span>
        <span class="muted mono" style="font-size:12px">mesmo grafo, mesma origem (1)</span>
      </div>
      <div class="grid-2">
        <div class="card" style="--alg:var(--vec);padding:12px">
          <div class="row"><b class="c-vec">Dijkstra com vetor</b><span class="spacer"></span><span class="mono muted st-v"></span></div>
          <div class="gw-canvas readonly cv-v" style="min-height:230px;flex:1"></div>
          <div class="stats">
            <div class="stat"><span class="v c-vec w-v">0</span><span class="l">comparações p/ achar o mínimo</span></div>
            <div class="stat"><span class="v p-v">0</span><span class="l">pops</span></div>
          </div>
        </div>
        <div class="card" style="--alg:var(--dij);padding:12px">
          <div class="row"><b class="c-dij">Dijkstra com heap</b><span class="spacer"></span><span class="mono muted st-h"></span></div>
          <div class="gw-canvas readonly cv-h" style="min-height:230px;flex:1"></div>
          <div class="stats">
            <div class="stat"><span class="v c-dij w-h">0</span><span class="l">comparações no heap</span></div>
            <div class="stat"><span class="v p-h">0</span><span class="l">pops <span class="d-h"></span></span></div>
          </div>
        </div>
      </div>`;
    const q = (s) => root.querySelector(s);
    const vv = new GraphView(q(".cv-v"), { showWeights: true });
    const vh = new GraphView(q(".cv-h"), { showWeights: true });
    let rv, rh, iv = 0, ih = 0, timer = null, n = 14;

    function newGraph() {
      const g = GraphGen.randomGraph(n, Math.round(n * 0.9), true);
      vv.setGraph(g, 1); vh.setGraph(g, 1);
      rv = Engine.run(vv.graph, 1, "dij-vetor");
      rh = Engine.run(vh.graph, 1, "dij-heap");
      iv = ih = 0;
      render();
    }
    function render() {
      const sv = rv.steps[iv], sh = rh.steps[ih];
      vv.render(sv); vh.render(sh);
      q(".w-v").textContent = fInt(sv.counters.work);
      q(".w-h").textContent = fInt(sh.counters.work);
      q(".p-v").textContent = sv.counters.pops;
      q(".p-h").textContent = sh.counters.pops;
      q(".d-h").textContent = sh.counters.discards ? `(${sh.counters.discards} descartados)` : "";
      q(".st-v").textContent = `${iv + 1}/${rv.steps.length}`;
      q(".st-h").textContent = `${ih + 1}/${rh.steps.length}`;
    }
    const atEnd = () => iv >= rv.steps.length - 1 && ih >= rh.steps.length - 1;
    function step() {
      iv = Math.min(rv.steps.length - 1, iv + 1);
      ih = Math.min(rh.steps.length - 1, ih + 1);
      render();
      if (atEnd()) stop();
    }
    function play() {
      if (atEnd()) { iv = ih = 0; }
      q("[data-a=play]").innerHTML = ICON.pause;
      timer = setInterval(step, 160);
    }
    function stop() { clearInterval(timer); timer = null; q("[data-a=play]").innerHTML = ICON.play; }
    q("[data-a=play]").addEventListener("click", () => (timer ? stop() : play()));
    q("[data-a=step]").addEventListener("click", () => { stop(); step(); });
    q("[data-a=end]").addEventListener("click", () => { stop(); iv = rv.steps.length - 1; ih = rh.steps.length - 1; render(); });
    q("[data-a=new]").addEventListener("click", () => { stop(); newGraph(); });
    q(".nsl").addEventListener("input", (e) => { n = +e.target.value; q(".nval").textContent = n; stop(); newGraph(); });
    newGraph();
    return { stop };
  }

  // ---------------------------------------------------- fundo da capa
  function CoverBackground(container) {
    const W = 1600, H = 900;
    const svg = d3.select(container).append("svg").attr("viewBox", `0 0 ${W} ${H}`).attr("preserveAspectRatio", "xMidYMid slice");
    const cols = [css("--bfs"), css("--dij"), css("--dfs")];
    const nodes = d3.range(70).map((i) => ({ i, x: Math.random() * W, y: Math.random() * H, c: cols[i % 3] }));
    const links = [];
    nodes.forEach((a, i) => {
      const near = nodes.map((b, j) => [Math.hypot(a.x - b.x, a.y - b.y), j]).sort((p, q) => p[0] - q[0]).slice(1, 3);
      near.forEach(([, j]) => { if (i < j) links.push({ source: i, target: j }); });
    });
    const L = svg.append("g").selectAll("line").data(links).join("line").attr("stroke", "#3a4459").attr("stroke-opacity", .55);
    const N = svg.append("g").selectAll("circle").data(nodes).join("circle").attr("r", (d) => 2.5 + (d.i % 4)).attr("fill", (d) => d.c).attr("fill-opacity", .7);
    const sim = d3.forceSimulation(nodes)
      .force("link", d3.forceLink(links).distance(120).strength(.05))
      .force("charge", d3.forceManyBody().strength(-30))
      .force("x", d3.forceX(W / 2).strength(.004))
      .force("y", d3.forceY(H / 2).strength(.006))
      .alphaTarget(0.02)
      .velocityDecay(0.6)
      .on("tick", () => {
        L.attr("x1", (d) => d.source.x).attr("y1", (d) => d.source.y).attr("x2", (d) => d.target.x).attr("y2", (d) => d.target.y);
        N.attr("cx", (d) => d.x).attr("cy", (d) => d.y);
      });
    // pulso de "busca" percorrendo o grafo
    let wave = null;
    function pulse() {
      const adj = nodes.map(() => []);
      links.forEach((l) => { adj[l.source.index].push(l.target.index); adj[l.target.index].push(l.source.index); });
      const start = Math.floor(Math.random() * nodes.length);
      const lvl = new Map([[start, 0]]);
      const q = [start];
      while (q.length) { const u = q.shift(); for (const v of adj[u]) if (!lvl.has(v)) { lvl.set(v, lvl.get(u) + 1); q.push(v); } }
      N.transition().delay((d) => (lvl.has(d.index) ? lvl.get(d.index) * 160 : 0)).duration(250).attr("fill-opacity", 1).attr("r", (d) => 4.5 + (d.i % 4))
        .transition().duration(900).attr("fill-opacity", .7).attr("r", (d) => 2.5 + (d.i % 4));
    }
    return {
      start() { sim.alpha(0.3).restart(); pulse(); wave = setInterval(pulse, 4200); },
      stop() { sim.stop(); clearInterval(wave); },
    };
  }

  window.Charts = {
    PerfChart, PerfBars, StripChart, DegreeChart, TreeChart, TheoryChart, DualRun, CoverBackground,
    memEstimate, SIZEOF, SERIES, fmt: { fInt, fNum, fMs, fBig, fBytes, fDur }, showTip, hideTip,
  };
})();
