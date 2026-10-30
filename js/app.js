/* Navegacao dos slides + inicializacao preguicosa de cada slide. */
(function () {
  const { fInt, fNum, fMs, fBig, fBytes, fDur } = Charts.fmt;
  const slides = [...document.querySelectorAll(".slide")];
  const G = (id) => DADOS.grafos.find((g) => g.id === id);
  const css = (v) => getComputedStyle(document.documentElement).getPropertyValue(v).trim();
  let cur = -1;

  const PRESETS = [
    { label: "Exemplo", graph: () => GraphGen.EXEMPLOS.busca },
    { label: "Grade", graph: () => GraphGen.gridGraph(4, 5) },
    { label: "Árvore", graph: () => GraphGen.treeGraph(4) },
    { label: "Ciclo", graph: () => GraphGen.cycleGraph(12) },
    { label: "Aleatório", icon: "dice", graph: () => GraphGen.randomGraph(14, 7, false) },
  ];

  // Estimativa do vetor onde ele nao foi executado: constante medida c = t / n²
  function vetorEstimado(E) {
    const medidos = E.grafos.filter((g) => g.vetor_ms != null);
    if (!medidos.length) return () => null;
    const maior = medidos.reduce((a, b) => (b.n > a.n ? b : a));
    const c = maior.vetor_ms / (maior.n * maior.n);
    return (g) => c * g.n * g.n;
  }

  // ===================================================================
  // Inicializadores: rodam na primeira visita e podem devolver
  // { enter, leave } para ligar/desligar animacoes.
  // ===================================================================
  const INIT = {
    cover(s) {
      const c = Scenes.Cover3D(s.querySelector(".cover-bg"));
      return { enter: c.start, leave: c.stop };
    },

    lib(s) { return Scenes.LibBuild(s.querySelector(".lib-host")); },

    mem(s) {
      const sel = s.querySelector(".mem-graph");
      sel.innerHTML = DADOS.grafos.map((g) => `<option value="${g.id}">${g.nome} · n=${fBig(g.n)}</option>`).join("") +
        `<option value="__custom">Personalizado (m = 10·n)</option>`;
      sel.value = "W_5";
      let T = "long double";
      const nSlider = s.querySelector(".mem-n");
      const REPS = [
        { k: "matriz", nome: "Matriz de <code>int</code>", sub: "n² × 4 bytes", cor: "var(--danger)" },
        { k: "bits", nome: "Matriz de bits", sub: "n² × 1 bit (vector&lt;bool&gt;)", cor: "var(--warn)" },
        { k: "lista", nome: "Lista de adjacência <span class='badge'>a nossa</span>", sub: "2m × sizeof(Edge&lt;T&gt;)", cor: "var(--ok)" },
        { k: "csr", nome: "Lista compacta (CSR)", sub: "offsets + vizinhos + pesos", cor: "var(--bfs)" },
      ];
      const bars = s.querySelector(".mem-bars");
      bars.innerHTML = REPS.map((r) => `<div class="mem-bar" data-k="${r.k}">
        <div class="top"><span class="name">${r.nome} <span class="muted mono" style="font-size:11.5px">${r.sub}</span></span><span class="val" style="color:${r.cor}"></span></div>
        <div class="track"><div class="fill" style="background:${r.cor}"></div><div class="ram" title="16 GB de RAM"></div></div></div>`).join("");
      const pct = (b) => Math.max(1, Math.min(100, ((Math.log2(Math.max(b, 1024)) - 10) / 50) * 100));
      const RAM = 16 * 2 ** 30;
      s.querySelectorAll(".ram").forEach((r) => { r.style.left = `${pct(RAM)}%`; });
      const shown = {};

      function update() {
        let n, m, nome;
        const custom = sel.value === "__custom";
        s.querySelector(".mem-custom").style.display = custom ? "" : "none";
        if (custom) { n = Math.round(10 ** +nSlider.value); m = 10 * n; nome = "grafo personalizado"; s.querySelector(".mn").textContent = fBig(n); }
        else { const g = G(sel.value); n = g.n; m = g.m; nome = g.nome; }
        const est = Charts.memEstimate(n, m, T);
        for (const r of REPS) {
          const b = bars.querySelector(`[data-k="${r.k}"]`);
          b.querySelector(".fill").style.width = `${pct(est[r.k])}%`;
          const valEl = b.querySelector(".val");
          const from = shown[r.k] || est[r.k] / 50;
          d3.select(valEl).transition().duration(700).tween("t", () => {
            const i = d3.interpolateNumber(Math.log(from), Math.log(est[r.k]));
            return (t) => { valEl.textContent = fBytes(Math.exp(i(t))); };
          });
          shown[r.k] = est[r.k];
          b.classList.toggle("over", est[r.k] > RAM);
        }
        const v = s.querySelector(".mem-verdict");
        if (est.bits > RAM) {
          v.className = "mem-verdict bad";
          v.innerHTML = `No <b>${nome}</b> nem a matriz de bits cabe: <b>${fBytes(est.bits)}</b>, cerca de ${fInt(Math.round(est.bits / RAM))}× a RAM. A lista ocupa <b>${fBytes(est.lista)}</b>.`;
        } else if (est.matriz > RAM) {
          v.className = "mem-verdict bad";
          v.innerHTML = `No <b>${nome}</b> a matriz de <code>int</code> não cabe (${fBytes(est.matriz)}). Em bits ela cabe (${fBytes(est.bits)}), mas ainda gasta ${fNum(est.bits / est.lista, 0)}× mais que a lista.`;
        } else {
          v.className = "mem-verdict good";
          v.innerHTML = `Aqui tudo cabe em 16 GB, mas a matriz gasta <b>${fNum(est.matriz / est.lista, 0)}×</b> mais memória que a lista.`;
        }
        const dens = (2 * m) / (n * (n - 1));
        s.querySelector(".mem-stats").innerHTML = `
          <div class="stat"><span class="v">${fBig(n)}</span><span class="l">vértices</span></div>
          <div class="stat"><span class="v">${fBig(m)}</span><span class="l">arestas</span></div>
          <div class="stat"><span class="v c-vec">${dens < 1e-4 ? dens.toExponential(1).replace(".", ",") : fNum(dens * 100, 3) + "%"}</span><span class="l">das células da matriz seriam usadas</span></div>`;
      }
      sel.addEventListener("change", update);
      nSlider.addEventListener("input", update);
      s.querySelectorAll(".mem-type button").forEach((b) => b.addEventListener("click", () => {
        T = b.dataset.t;
        s.querySelectorAll(".mem-type button").forEach((x) => x.classList.toggle("on", x === b));
        update();
      }));
      update();
      return Scenes.SparseGrid(s.querySelector(".sparse-host"));
    },

    bfs(s) {
      const w = AlgoWidget(s.querySelector(".w-bfs"), { graph: GraphGen.EXEMPLOS.busca, alg: "bfs", presets: PRESETS });
      return { enter: w.replay, leave: w.stop };
    },

    dfs(s) {
      const w = AlgoWidget(s.querySelector(".w-dfs"), { graph: GraphGen.EXEMPLOS.busca, alg: "dfs", presets: PRESETS });
      return { enter: w.replay, leave: w.stop };
    },

    frontier(s) { return Scenes.QuadRun(s.querySelector(".quad-host")); },

    dijkstra(s) {
      const w = AlgoWidget(s.querySelector(".w-dij"), {
        graph: GraphGen.EXEMPLOS.pdf, alg: "dij-heap", algs: ["dij-heap", "dij-vetor"],
        presets: [
          { label: "Figura 1", graph: () => GraphGen.EXEMPLOS.pdf },
          { label: "Exemplo maior", graph: () => GraphGen.EXEMPLOS.dijkstra },
          { label: "Aleatório", icon: "dice", graph: () => GraphGen.randomGraph(10, 8, true) },
        ],
      });
      return { leave: w.stop };
    },

    dual(s) { return Scenes.VecHeap3D(s.querySelector(".vh-host")); },

    theory(s) {
      Charts.TheoryChart(s.querySelector(".theory-chart"));
      const rows = DADOS.grafos.slice().sort((a, b) => a.n - b.n).map((g) => {
        const v = g.n * g.n * 1e-9, h = (g.n + g.m) * Math.log2(g.n) * 1e-9;
        return `<tr><td>${g.nome}</td><td>${fBig(g.n)}</td><td class="c-dij">${fDur(h)}</td><td class="c-vec">${fDur(v)}</td></tr>`;
      });
      s.querySelector(".theory-table").innerHTML =
        `<thead><tr><th>grafo</th><th>n</th><th>heap</th><th>vetor</th></tr></thead><tbody>${rows.join("")}</tbody>`;
    },

    graphs(s) {
      const deg = Charts.DegreeChart(s.querySelector(".deg-chart"));
      const tbl = s.querySelector(".graphs-table");
      tbl.innerHTML = `<thead><tr><th>grafo</th><th>n</th><th>m</th><th>grau médio</th><th>min–máx</th><th>comp.</th></tr></thead><tbody>${
        DADOS.grafos.map((g) => `<tr data-id="${g.id}"><td>${g.nome}${g.ponderado ? '<span class="badge w">pesos</span>' : ""}</td>
          <td>${fInt(g.n)}</td><td>${fInt(g.m)}</td><td>${fNum(g.graus.medio, 2)}</td>
          <td>${g.graus.minimo}–${fInt(g.graus.maximo)}</td><td>${fInt(g.componentes.quantidade)}</td></tr>`).join("")
      }</tbody>`;
      function pick(id) {
        const g = G(id);
        tbl.querySelectorAll("tbody tr").forEach((r) => r.classList.toggle("sel", r.dataset.id === id));
        s.querySelector(".deg-title").textContent = `Graus · ${g.nome}`;
        deg.show(id);
        s.querySelector(".deg-stats").innerHTML = `
          <div class="stat"><span class="v">${g.graus.minimo}</span><span class="l">grau mínimo</span></div>
          <div class="stat"><span class="v">${fInt(g.graus.maximo)}</span><span class="l">grau máximo</span></div>
          <div class="stat"><span class="v">${fNum(g.graus.mediano, 1)}</span><span class="l">mediana</span></div>
          <div class="stat"><span class="v">${fNum((100 * g.componentes.maiores[0]) / g.n, 1)}%</span><span class="l">na maior componente</span></div>`;
      }
      tbl.querySelectorAll("tbody tr").forEach((r) => r.addEventListener("click", () => pick(r.dataset.id)));
      s.querySelectorAll(".deg-scale button").forEach((b) => b.addEventListener("click", () => {
        s.querySelectorAll(".deg-scale button").forEach((x) => x.classList.toggle("on", x === b));
        deg.setLog(b.dataset.s === "log");
      }));
      pick("rede_colaboracao");
    },

    perf(s) {
      const ratio = (g) => g.benchmark.Dijkstra_Heap.media / g.benchmark.BFS.media;
      const pond = DADOS.grafos.filter((g) => g.ponderado).map(ratio);
      const sem = DADOS.grafos.filter((g) => !g.ponderado).map(ratio);
      s.querySelector(".perf-lead").innerHTML = `<b>Sim, nos grafos com pesos:</b> o Dijkstra levou de <b class="c-warn">${fNum(d3.min(pond), 1)}× a ${fNum(d3.max(pond), 1)}×</b> o tempo da BFS.
        Cada passo do heap faz comparações em O(log n) e acumula entradas descartadas, enquanto a fila e a pilha fazem O(1).
        Nos grafos <b>sem peso</b> (todo peso = 1) a diferença é pequena: de ${fNum(d3.min(sem), 1)}× a ${fNum(d3.max(sem), 1)}×. A BFS e a DFS praticamente empatam sempre.`;
      const strip = Charts.StripChart(s.querySelector(".strip-chart"));
      function pick(id) {
        const g = G(id);
        s.querySelector(".strip-title").textContent = `${g.nome} · as 100 execuções`;
        strip.show(id);
        const b = g.benchmark;
        s.querySelector(".strip-stats").innerHTML = Charts.SERIES.map((x) =>
          `<div class="stat"><span class="v" style="color:${x.cor()}">${fMs(b[x.key].media)}</span><span class="l">${x.nome}</span></div>`).join("");
      }
      const chart = Charts.PerfBars(s.querySelector(".perf-chart"), { onSelect: pick, selected: "W_5" });
      s.querySelector(".perf-legend").innerHTML = Charts.SERIES.map((x) => `<button><span class="sw" style="color:${x.cor()}"></span>${x.nome}</button>`).join("");
      s.querySelectorAll(".perf-stat button").forEach((b) => b.addEventListener("click", () => {
        s.querySelectorAll(".perf-stat button").forEach((x) => x.classList.toggle("on", x === b));
        chart.setStat(b.dataset.s);
      }));
      pick("W_5");
    },

    trees(s) {
      const sel = s.querySelector(".tree-graph");
      sel.innerHTML = DADOS.grafos.map((g) => `<option value="${g.id}">${g.nome}</option>`).join("");
      sel.value = "W_3";
      let alg = "bfs";
      const chart = Scenes.ForceTree(s.querySelector(".tree-chart"), (i) => {
        s.querySelector(".explica").innerHTML = Scenes.EXPLICA[alg](i);
        s.querySelector(".tree-stats").innerHTML = `
          <div class="stat"><span class="v">${i.profundidade}</span><span class="l">profundidade máxima</span></div>
          <div class="stat"><span class="v">${i.filhosRaiz}</span><span class="l">filhos da origem</span></div>
          <div class="stat"><span class="v">${i.folhas}</span><span class="l">folhas (de ${i.total})</span></div>`;
      });
      const show = () => chart.show(sel.value, alg);
      const replay = s.querySelector(".tree-replay");
      replay.innerHTML = `${ICON.reset} reproduzir`;
      replay.addEventListener("click", show);
      sel.addEventListener("change", show);
      s.querySelectorAll(".tree-alg button").forEach((b) => b.addEventListener("click", () => {
        alg = b.dataset.a;
        s.querySelectorAll(".tree-alg button").forEach((x) => x.classList.toggle("on", x === b));
        show();
      }));
      return { enter() { requestAnimationFrame(show); }, leave: chart.stop };
    },

    gallery(s) {
      const sel = s.querySelector(".x3d-graph");
      sel.innerHTML = DADOS.grafos.map((g) => `<option value="${g.id}">${g.nome}</option>`).join("");
      sel.value = "W_3";
      const algs = new Set(["bfs", "dfs", "dijkstra"]);
      const ex = Scenes.Explorer3D(s.querySelector(".x3d"), (st) => {
        s.querySelector(".x3d-stats").innerHTML = `
          <div class="stat"><span class="v">${fInt(st.nodes)}</span><span class="l">vértices</span></div>
          <div class="stat"><span class="v">${fInt(st.links)}</span><span class="l">arestas de árvore</span></div>
          <div class="stat"><span class="v">${fInt(st.shared)}</span><span class="l">em mais de uma árvore</span></div>`;
      });
      const nIn = s.querySelector(".x3d-n");
      const show = () => ex.show(sel.value, [...algs], +nIn.value);
      sel.addEventListener("change", show);
      nIn.addEventListener("change", show);
      nIn.addEventListener("input", () => { s.querySelector(".x3d-nv").textContent = nIn.value; });
      s.querySelector(".x3d-rot").addEventListener("change", (e) => ex.rotate(e.target.checked));
      s.querySelectorAll(".x3d-algs button").forEach((b) => b.addEventListener("click", () => {
        if (algs.has(b.dataset.a) && algs.size > 1) algs.delete(b.dataset.a); else algs.add(b.dataset.a);
        s.querySelectorAll(".x3d-algs button").forEach((x) => x.classList.toggle("on", algs.has(x.dataset.a)));
        show();
      }));
      // aba de imagens
      const box = s.querySelector(".gallery");
      const ordem = DADOS.grafos.map((g) => g.id);
      box.innerHTML = DADOS.sobreposicoes.slice().sort((a, b) => ordem.indexOf(a.id) - ordem.indexOf(b.id))
        .map((i) => `<button class="thumb" data-src="${i.arquivo}" data-t="${G(i.id).nome}"><img src="${i.arquivo}" alt="" loading="lazy" /><span>${G(i.id).nome}</span></button>`).join("");
      box.querySelectorAll(".thumb").forEach((t) => t.addEventListener("click", () => Lightbox.open(t.dataset.src, t.dataset.t)));
      let aba = "3d";
      s.querySelectorAll(".x3d-tab button").forEach((b) => b.addEventListener("click", () => {
        s.querySelectorAll(".x3d-tab button").forEach((x) => x.classList.toggle("on", x === b));
        aba = b.dataset.t;
        const img = aba === "img";
        s.querySelector(".x3d").style.display = img ? "none" : "";
        s.querySelector(".gallery-wrap").style.display = img ? "" : "none";
        img ? ex.leave() : ex.enter();
      }));
      show();
      return { enter() { if (aba === "3d") ex.enter(); }, leave: ex.leave };
    },

    e1(s) {
      const body = s.querySelector(".e1-body");
      const E = window.ESTUDOS && ESTUDOS.estudo1;
      if (!E) return placeholder(body, "estudo1");
      const tabs = E.grafos.map((g, i) => `<button data-i="${i}" class="${i ? "" : "on"}">${g.nome}</button>`).join("");
      body.innerHTML = `<div class="row"><div class="seg e1-tabs">${tabs}</div><span class="spacer"></span><span class="mono muted e1-meta" style="font-size:12px"></span></div>
        <div class="grid-2-wide e1-grid"><div class="card"><div class="table-wrap"><table class="dtable e1-table"></table></div></div>
        <div class="card"><h3>distância de 10 até cada destino · escala log</h3><div class="chart e1-chart"></div></div></div>`;
      const chart = distChart(body.querySelector(".e1-chart"), E);
      const show = (i) => {
        const g = E.grafos[i];
        body.querySelector(".e1-meta").textContent = g.erro ? "" : `n = ${fBig(g.n)} · m = ${fBig(g.m)} · Dijkstra (heap) em ${fMs(g.tempo_s * 1000)}`;
        body.querySelector(".e1-table").innerHTML = g.erro ? `<tbody><tr><td class="c-danger">${g.erro}</td></tr></tbody>` :
          `<thead><tr><th>destino</th><th>distância</th><th>arestas</th><th style="text-align:left">caminho mínimo</th></tr></thead><tbody>${
            g.destinos.map((d, r) => `<tr><td>10 → ${d.destino}</td>
              <td class="c-dij">${d.distancia == null ? "∞" : fNum(d.distancia, 4)}</td>
              <td>${d.caminho ? d.caminho.length - 1 : "—"}</td>
              <td style="text-align:left;white-space:normal">${d.caminho ? pathHTML(d.caminho, r * 4) : '<span class="muted">inalcançável</span>'}</td></tr>`).join("")
          }</tbody>`;
        chart.highlight(g.id);
      };
      body.querySelectorAll(".e1-tabs button").forEach((b) => b.addEventListener("click", () => {
        body.querySelectorAll(".e1-tabs button").forEach((x) => x.classList.toggle("on", x === b));
        show(+b.dataset.i);
      }));
      show(0);
    },

    e2(s) {
      const body = s.querySelector(".e2-body");
      const E = window.ESTUDOS && ESTUDOS.estudo2;
      if (!E) return placeholder(body, "estudo2");
      const est = vetorEstimado(E);
      const gs = E.grafos.map((g) => ({ ...g, vetor_est: g.vetor_ms == null ? est(g) : null }));
      const medidos = gs.filter((g) => g.vetor_ms != null);
      s.querySelector(".e2-lead").innerHTML = `Sorteamos até <b>${E.k_pedido}</b> vértices iniciais por grafo (semente fixa) e medimos cada execução.
        O <b class="c-vec">vetor</b> é O(n²), então só rodou até n = ${fBig(d3.max(medidos, (g) => g.n) || 0)}, com limite de tempo.
        Nos grafos maiores o tempo dele foi <b>estimado</b> pela constante medida (t ÷ n²). O k usado em cada caso aparece na tabela.`;
      body.innerHTML = `<div class="grid-2-wide"><div class="card"><div class="chart e2-chart"></div>
          <div class="legend"><button><span class="sw" style="color:var(--vec)"></span>vetor (medido)</button><button><span class="sw hatch"></span>vetor (estimado)</button><button><span class="sw" style="color:var(--dij)"></span>heap</button></div></div>
        <div class="card"><div class="table-wrap"><table class="dtable e2-table"></table></div>
          <p class="note">${medidos.length ? `Conferência: na primeira origem, vetor e heap deram as mesmas distâncias (diferença máxima ${medidos.map((g) => (g.diferenca == null || g.diferenca < 0 ? "—" : g.diferenca === 0 ? "0" : g.diferenca.toExponential(0))).join(", ")}).` : ""}</p></div></div>`;
      body.querySelector(".e2-table").innerHTML = `<thead><tr><th>grafo</th><th>heap</th><th>vetor</th><th>heap é</th></tr></thead><tbody>${
        gs.map((g) => {
          const v = g.vetor_ms ?? g.vetor_est;
          return `<tr><td>${g.nome}</td><td class="c-dij">${fMs(g.heap_ms)}<span class="badge">k=${g.k_heap}</span></td>
          <td class="c-vec">${g.vetor_ms != null ? `${fDur(g.vetor_ms / 1000)}<span class="badge">k=${g.k_vetor}</span>` : `≈ ${fDur(g.vetor_est / 1000)}<span class="badge">estimado</span>`}</td>
          <td><b>${fInt(Math.round(v / g.heap_ms))}×</b> mais rápido</td></tr>`;
        }).join("")
      }</tbody>`;
      e2Bars(body.querySelector(".e2-chart"), gs);
    },

    e3(s) {
      const body = s.querySelector(".e3-body");
      const E = window.ESTUDOS && ESTUDOS.estudo3;
      if (!E) return placeholder(body, "estudo3");
      body.innerHTML = `<div class="grid-2-wide"><div class="card" style="padding:8px"><div class="chart rg"></div></div>
        <div class="card"><div class="e3-list"></div></div></div>`;
      const host = body.querySelector(".rg");
      Scenes.ResearchGraph(host, E);
      body.querySelector(".e3-list").innerHTML = E.destinos.map((d) => `
        <div class="e3-item" data-n="${d.destino}"><div class="row"><b>${d.destino}</b><span class="spacer"></span>
          <span class="mono c-bfs">${d.distancia == null ? "∞" : fNum(d.distancia, 3)}</span></div>
          ${d.caminho ? `<span class="note">${d.caminho.length - 1} arestas</span>${pathHTML(d.caminho)}` : `<span class="note"><b class="c-danger">Sem caminho:</b> está em outra componente conexa.${d.coautores && d.coautores.length
            ? ` Na rede, ${d.coautores.length > 1 ? "os coautores são" : "o único coautor é"} <b>${d.coautores.join(", ")}</b>, e ${d.coautores.length > 1 ? "eles" : "os dois"} não se ligam ao resto.` : ""}</span>`}</div>`).join("");
      body.querySelectorAll(".e3-item").forEach((it) => {
        it.addEventListener("mouseenter", () => host.highlight && host.highlight(it.dataset.n));
        it.addEventListener("mouseleave", () => host.highlight && host.highlight(null));
      });
    },

    end(s) {
      const W5 = G("W_5");
      const est = Charts.memEstimate(W5.n, W5.m, "long double");
      const ratios = DADOS.grafos.filter((g) => g.ponderado).map((g) => g.benchmark.Dijkstra_Heap.media / g.benchmark.BFS.media);
      const cards = [
        `<div class="takeaway"><span class="big c-dij">1 laço</span><p><b>Quatro fronteiras.</b> BFS, DFS, Dijkstra com vetor e Dijkstra com heap usam o mesmo <code>traverse_impl</code>. Só muda quem sai primeiro.</p></div>`,
        `<div class="takeaway"><span class="big c-ok">${fBytes(est.lista)}</span><p>É a <b>lista de adjacência</b> do W_5. A matriz precisaria de <b class="c-danger">${fBytes(est.matriz)}</b>, e mesmo em bits seriam ${fBytes(est.bits)}.</p></div>`,
        `<div class="takeaway"><span class="big c-vec">${fNum(d3.min(ratios), 1)}–${fNum(d3.max(ratios), 1)}×</span><p>É quanto o <b>Dijkstra com heap</b> custa a mais que a BFS nos grafos com pesos.</p></div>`,
      ];
      const E2 = window.ESTUDOS && ESTUDOS.estudo2;
      const medidos = E2 ? E2.grafos.filter((g) => g.vetor_ms != null).sort((a, b) => b.n - a.n) : [];
      if (medidos.length) {
        const big = medidos[0];
        cards.push(`<div class="takeaway"><span class="big c-warn">${fInt(Math.round(big.vetor_ms / big.heap_ms))}×</span><p>No <b>${big.nome}</b> (n = ${fBig(big.n)}), o heap foi esse tanto mais rápido que o vetor: ${fMs(big.heap_ms)} contra ${fDur(big.vetor_ms / 1000)} por execução.</p></div>`);
      } else {
        cards.push(`<div class="takeaway"><span class="big c-warn">${fDur(W5.n * W5.n * 1e-9)}</span><p>Estimativa para um único <b>Dijkstra com vetor</b> no W_5 (n² a 1 ns/op).</p></div>`);
      }
      const E3 = window.ESTUDOS && ESTUDOS.estudo3;
      if (E3) {
        const perto = E3.destinos.filter((d) => d.distancia != null).sort((a, b) => a.distancia - b.distancia)[0];
        if (perto) cards.push(`<div class="takeaway"><span class="big c-bfs">${perto.caminho.length - 1} passos</span><p>Separam <b>Edsger W. Dijkstra</b> de <b>${perto.destino}</b>, o mais próximo dele entre os cinco pesquisadores (distância ${fNum(perto.distancia, 3)}).</p></div>`);
      }
      s.querySelector(".end-cards").innerHTML = cards.join("");
    },
  };

  // ------------------------------------------------------ auxiliares
  const FORMATOS = {
    estudo1: "Rodar dijkstra_heap(10) em cada grafo com pesos (ver Estudos/run_estudos.sh).",
    estudo2: "Medir dijkstra_vector e dijkstra_heap a partir de vértices sorteados (ver Estudos/run_estudos.sh).",
    estudo3: "Rodar dijkstra_heap a partir de Edsger W. Dijkstra na rede de colaboração (ver Estudos/run_estudos.sh).",
  };
  function placeholder(body, k) {
    body.innerHTML = `<div class="placeholder"><div><span class="status"><span class="sw"></span>calculando</span>
      <h4>Os números ainda estão sendo gerados</h4><p>${FORMATOS[k]}</p>
      <p class="note">Depois: <code>python site/tools/build_estudos.py</code> e recarregue a página.</p></div><div></div></div>`;
  }

  function pathHTML(p, delay0 = 0) {
    let items = p.map(String);
    if (items.length > 14) items = [...items.slice(0, 6), `… +${items.length - 12} …`, ...items.slice(-6)];
    return `<span class="path ${typeof p[0] === "string" ? "names" : ""}">${items.map((v, i) =>
      `${i ? `<span class="arr" style="--i:${delay0 + i}">→</span>` : ""}<span class="pv ${i === items.length - 1 ? "end" : ""}" style="--i:${delay0 + i}">${v}</span>`).join("")}</span>`;
  }

  // Estudo 1: distancia por destino, uma linha por grafo
  function distChart(container, E) {
    const svg = d3.select(container).append("svg");
    let hl = null, size = null;
    const cores = d3.scaleOrdinal().domain(E.grafos.map((g) => g.id)).range([css("--bfs"), css("--dij"), css("--vec"), css("--dfs"), css("--ok"), "#c792ea"]);
    function draw(w, h) {
      size = [w, h];
      svg.selectAll("*").remove();
      const ok = E.grafos.filter((g) => !g.erro);
      const destinos = ok[0] ? ok[0].destinos.map((d) => d.destino) : [];
      const m = { t: 14, r: 50, b: 30, l: 44 };
      const x = d3.scalePoint().domain(destinos).range([m.l, w - m.r]).padding(0.3);
      const vals = ok.flatMap((g) => g.destinos.map((d) => d.distancia).filter((v) => v != null));
      // escala log: as distancias vao de < 1 (W_3) a ~100 (W_7)
      const y = d3.scaleLog().domain([d3.min(vals) * 0.7, d3.max(vals) * 1.4]).range([h - m.b, m.t]);
      const ticks = y.ticks().filter((t) => /^[125]/.test(String(t)));
      svg.append("g").attr("class", "gridline").attr("transform", `translate(${m.l},0)`).call(d3.axisLeft(y).tickValues(ticks).tickSize(-(w - m.l - m.r)).tickFormat(""));
      svg.append("g").attr("class", "axis").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).tickFormat((d) => `10→${d}`));
      svg.append("g").attr("class", "axis").attr("transform", `translate(${m.l},0)`).call(d3.axisLeft(y).tickValues(ticks).tickFormat((d) => fNum(d, d < 1 ? 1 : 0)));
      for (const g of ok) {
        const pts = g.destinos.filter((d) => d.distancia != null);
        const on = !hl || hl === g.id;
        const p = svg.append("path").datum(pts).attr("fill", "none").attr("stroke", cores(g.id)).attr("stroke-width", on ? 2.6 : 1.2).attr("stroke-opacity", on ? 1 : 0.25)
          .attr("d", d3.line().x((d) => x(d.destino)).y((d) => y(d.distancia)));
        const len = p.node().getTotalLength();
        p.attr("stroke-dasharray", `${len} ${len}`).attr("stroke-dashoffset", len).transition().duration(800).attr("stroke-dashoffset", 0);
        svg.append("g").selectAll("circle").data(pts).join("circle").attr("cx", (d) => x(d.destino)).attr("cy", (d) => y(d.distancia))
          .attr("r", on ? 4.5 : 3).attr("fill", cores(g.id)).attr("fill-opacity", on ? 1 : 0.3)
          .on("mousemove", (ev, d) => Charts.showTip(ev, `<b>${g.nome}</b><br>10 → ${d.destino}: <b>${fNum(d.distancia, 4)}</b><br>${d.caminho.length - 1} arestas`))
          .on("mouseleave", Charts.hideTip);
        const last = pts[pts.length - 1];
        if (last) svg.append("text").attr("x", x(last.destino) + 8).attr("y", y(last.distancia)).attr("dy", "0.35em").attr("fill", cores(g.id))
          .attr("font-size", 11).attr("font-family", "var(--font-mono)").attr("opacity", on ? 1 : 0.35).text(g.id);
      }
    }
    new ResizeObserver(() => {
      const r = container.getBoundingClientRect();
      if (r.width > 10 && r.height > 10) draw(r.width, r.height);
    }).observe(container);
    return { highlight(id) { hl = id; if (size) draw(...size); } };
  }

  // Estudo 2: barras log, vetor medido/estimado x heap
  function e2Bars(container, data) {
    const svg = d3.select(container).append("svg");
    svg.append("defs").append("pattern").attr("id", "hatch").attr("width", 6).attr("height", 6).attr("patternUnits", "userSpaceOnUse").attr("patternTransform", "rotate(45)")
      .append("rect").attr("width", 3).attr("height", 6).attr("fill", css("--vec")).attr("fill-opacity", 0.7);
    new ResizeObserver(() => {
      const { width: w, height: h } = container.getBoundingClientRect();
      if (w < 10 || h < 10) return;
      svg.selectAll("g").remove();
      const m = { t: 10, r: 80, b: 30, l: 80 };
      const vet = (d) => d.vetor_ms ?? d.vetor_est;
      const y0 = d3.scaleBand().domain(data.map((d) => d.nome)).range([m.t, h - m.b]).padding(0.25);
      const y1 = d3.scaleBand().domain(["vetor", "heap"]).range([0, y0.bandwidth()]).padding(0.12);
      const x = d3.scaleLog().domain([d3.min(data, (d) => d.heap_ms) / 2, d3.max(data, vet) * 3]).range([m.l, w - m.r]);
      svg.append("g").attr("class", "gridline").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).ticks(5).tickSize(-(h - m.t - m.b)).tickFormat(""));
      svg.append("g").attr("class", "axis").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).ticks(5).tickFormat((d) => {
        const e = Math.log10(d);
        return Math.abs(e - Math.round(e)) < 1e-6 && Math.round(e) % 2 === 0 ? fDur(d / 1000) : "";
      }));
      svg.append("g").attr("class", "axis").attr("transform", `translate(${m.l},0)`).call(d3.axisLeft(y0).tickSize(0)).select(".domain").remove();
      const row = svg.append("g").selectAll("g").data(data).join("g").attr("transform", (d) => `translate(0,${y0(d.nome)})`);
      const bar = (k, val, fill) => row.append("rect").attr("x", m.l).attr("y", y1(k)).attr("height", y1.bandwidth()).attr("rx", 4).attr("fill", fill)
        .attr("width", 0).transition().duration(900).delay((d, i) => i * 90).attr("width", (d) => Math.max(2, x(val(d)) - m.l));
      bar("vetor", vet, (d) => (d.vetor_ms != null ? css("--vec") : "url(#hatch)"));
      bar("heap", (d) => d.heap_ms, css("--dij"));
      row.append("text").attr("x", (d) => x(vet(d)) + 6).attr("y", y1("vetor") + y1.bandwidth() / 2).attr("dy", "0.35em").attr("fill", css("--muted"))
        .attr("font-size", 11).attr("font-family", "var(--font-mono)").text((d) => `${d.vetor_ms != null ? "" : "≈ "}${fDur(vet(d) / 1000)}`);
      row.append("text").attr("x", (d) => x(d.heap_ms) + 6).attr("y", y1("heap") + y1.bandwidth() / 2).attr("dy", "0.35em").attr("fill", css("--muted"))
        .attr("font-size", 11).attr("font-family", "var(--font-mono)").text((d) => fMs(d.heap_ms));
    }).observe(container);
  }

  // ------------------------------------------------------- lightbox
  const Lightbox = (() => {
    const lb = document.querySelector(".lightbox");
    const img = lb.querySelector("img");
    const stage = d3.select(lb.querySelector(".stage"));
    const zoom = d3.zoom().scaleExtent([0.05, 12]).on("zoom", (e) => {
      img.style.transform = `translate(${e.transform.x}px,${e.transform.y}px) scale(${e.transform.k})`;
    });
    stage.call(zoom);
    const close = () => lb.classList.remove("open");
    lb.querySelector(".lb-close").addEventListener("click", close);
    return {
      open(src, title) {
        lb.querySelector(".lb-title").textContent = title;
        lb.classList.add("open");
        img.onload = () => {
          const W = innerWidth, H = innerHeight - 60;
          const k = Math.min(W / img.naturalWidth, H / img.naturalHeight);
          stage.call(zoom.transform, d3.zoomIdentity.translate((W - img.naturalWidth * k) / 2, 60 + (H - img.naturalHeight * k) / 2).scale(k));
        };
        img.src = src;
      },
      close,
      isOpen: () => lb.classList.contains("open"),
    };
  })();

  // ===================================================================
  // Deck
  // ===================================================================
  const hooks = new Map();
  const progress = document.querySelector(".progress");
  document.querySelector(".tot").textContent = slides.length;

  function go(i, push = true) {
    i = Math.max(0, Math.min(slides.length - 1, i));
    if (i === cur) return;
    const prev = cur;
    if (prev >= 0 && hooks.get(prev) && hooks.get(prev).leave) { try { hooks.get(prev).leave(); } catch (err) { console.error(err); } }
    cur = i;
    slides.forEach((s, k) => {
      s.classList.toggle("is-active", k === i);
      s.classList.toggle("is-before", k < i);
    });
    progress.style.width = `${((i + 1) / slides.length) * 100}%`;
    document.querySelector(".navbar .cur").textContent = i + 1;
    if (push) history.replaceState(null, "", `#${i + 1}`);

    const s = slides[i];
    if (!hooks.has(i)) {
      const fn = INIT[s.dataset.init];
      let h = null;
      try { h = fn ? fn(s) : null; } catch (err) {
        console.error(`slide ${i + 1}:`, err);
        (window.__errs = window.__errs || []).push(`slide ${i + 1}: ${err.message}`);
        document.documentElement.setAttribute("data-errs", window.__errs.join(" | "));
      }
      hooks.set(i, h || {});
    }
    const h = hooks.get(i);
    if (h.enter) { try { h.enter(); } catch (err) { console.error(err); } }
  }

  const typing = (t) => t && (t.tagName === "INPUT" || t.tagName === "SELECT" || t.tagName === "TEXTAREA");
  document.addEventListener("keydown", (e) => {
    if (typing(e.target) || e.ctrlKey || e.metaKey || e.altKey) return;
    const ov = document.querySelector(".overview");
    if (e.key === "Escape") {
      if (Lightbox.isOpen()) Lightbox.close();
      ov.classList.remove("open");
      return;
    }
    if (Lightbox.isOpen()) return;
    if (["ArrowRight", "PageDown"].includes(e.key)) { e.preventDefault(); go(cur + 1); }
    else if (["ArrowLeft", "PageUp"].includes(e.key)) { e.preventDefault(); go(cur - 1); }
    else if (e.key === "Home") go(0);
    else if (e.key === "End") go(slides.length - 1);
    else if (e.key === "o" || e.key === "O") toggleOverview();
    else if (e.key === "f" || e.key === "F") toggleFull();
    else if (e.key === "t" || e.key === "T") Timer.toggle();
  });

  let tx = null, ty = null;
  document.addEventListener("touchstart", (e) => {
    if (e.target.closest(".gw-canvas, .chart, input, .lightbox, .x3d, .vh-3d, .cover-bg, .tree-chart")) { tx = null; return; }
    tx = e.touches[0].clientX; ty = e.touches[0].clientY;
  }, { passive: true });
  document.addEventListener("touchend", (e) => {
    if (tx == null) return;
    const dx = e.changedTouches[0].clientX - tx, dy = e.changedTouches[0].clientY - ty;
    if (Math.abs(dx) > 70 && Math.abs(dx) > Math.abs(dy) * 1.5) go(cur + (dx < 0 ? 1 : -1));
    tx = null;
  });

  document.querySelector(".nav-prev").addEventListener("click", () => go(cur - 1));
  document.querySelector(".nav-next").addEventListener("click", () => go(cur + 1));
  document.querySelector(".nav-overview").addEventListener("click", () => toggleOverview());
  document.querySelector(".nav-full").addEventListener("click", () => toggleFull());

  function toggleFull() {
    if (document.fullscreenElement) document.exitFullscreen();
    else document.documentElement.requestFullscreen && document.documentElement.requestFullscreen();
  }

  function toggleOverview() {
    const ov = document.querySelector(".overview");
    const grid = ov.querySelector(".overview-grid");
    grid.innerHTML = slides.map((s, i) => `<button class="overview-item ${i === cur ? "current" : ""}" data-i="${i}">
      <span class="n">${String(i + 1).padStart(2, "0")}</span><span class="t">${s.dataset.title}</span></button>`).join("");
    grid.querySelectorAll(".overview-item").forEach((b) => b.addEventListener("click", () => { ov.classList.remove("open"); go(+b.dataset.i); }));
    ov.classList.toggle("open");
  }

  const Timer = (() => {
    const el = document.querySelector(".timer");
    const tv = el.querySelector(".tv");
    const TOTAL = 8 * 60;
    let left = TOTAL, t = null;
    const fmt = (s) => {
      const a = Math.abs(s);
      return `${s < 0 ? "+" : ""}${String(Math.floor(a / 60)).padStart(2, "0")}:${String(a % 60).padStart(2, "0")}`;
    };
    const paint = () => {
      tv.textContent = fmt(left);
      el.classList.toggle("running", !!t);
      el.classList.toggle("late", left <= 60 && left >= 0);
      el.classList.toggle("over", left < 0);
    };
    const toggle = () => {
      if (t) { clearInterval(t); t = null; }
      else t = setInterval(() => { left--; paint(); }, 1000);
      paint();
    };
    el.addEventListener("click", toggle);
    el.addEventListener("dblclick", () => { if (t) clearInterval(t); t = null; left = TOTAL; paint(); });
    paint();
    return { toggle };
  })();

  const start = parseInt(location.hash.slice(1), 10);
  go(Number.isFinite(start) ? start - 1 : 0, false);
})();
