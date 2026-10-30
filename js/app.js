/* Navegacao dos slides + inicializacao preguicosa de cada slide. */
(function () {
  const { fInt, fNum, fMs, fBig, fDur } = Charts.fmt;
  const slides = [...document.querySelectorAll(".slide")];
  const G = (id) => DADOS.grafos.find((g) => g.id === id);
  const css = (v) => getComputedStyle(document.documentElement).getPropertyValue(v).trim();
  let cur = -1;

  // Os dois vertices de maior grau da rede de colaboracao
  const GIGANTES = [{ grau: 643, nome: "Wei Wang" }, { grau: 605, nome: "Wei Li" }];

  // Estudo 3: grupos de pesquisadores, um grafo por aba
  const GRUPOS_E3 = [
    { label: "Pedidos no enunciado", nomes: ["Alan M. Turing", "J. B. Kruskal", "Jon M. Kleinberg", "Éva Tardos", "Daniel R. Figueiredo"] },
    { label: "Caminhos mínimos", nomes: ["Richard Bellman", "Robert W. Floyd", "Stephen Warshall", "Robert Endre Tarjan", "Leonhard Euler"],
      nota: "Os nomes por trás de Bellman-Ford, Floyd-Warshall e Tarjan. Euler aparece no DBLP, mas sem nenhum coautor." },
    { label: "Lendas", nomes: ["Edsger W. Dijkstra", "Donald E. Knuth", "Paul Erdös", "Richard M. Karp", "Claude E. Shannon", "John von Neumann"] },
    { label: "UFRJ · algoritmos e grafos", nomes: ["Valmir C. Barbosa", "Celina M. Herrera de Figueiredo", "Jayme Luiz Szwarcfiter", "Sulamita Klein", "Fábio Protti", "Nelson Maculan"] },
    { label: "UFRJ · sistemas", nomes: ["Edmundo de Souza e Silva", "Rosa M. M. Leão", "Daniel Sadoc Menasché", "Felipe M. G. França", "Claudio Esperança", "Ricardo Marroquim", "Geraldo Xexéo", "Guilherme Horta Travassos"] },
    { label: "Os dois gigantes", nomes: ["Wei Wang", "Wei Li"],
      nota: "<b>Wei Wang</b> (643 coautores) e <b>Wei Li</b> (605) são os vértices de maior grau da rede. Muito provavelmente nenhum dos dois é uma pessoa só: são nomes muito comuns, e pesquisadores homônimos acabam somados num único vértice. Escolha um deles como origem para ver como ficam perto de todo mundo." },
  ];

  // ===================================================================
  // Inicializadores: rodam na primeira visita e podem devolver
  // { enter, leave } para ligar/desligar animacoes.
  // ===================================================================
  const INIT = {
    cover(s) {
      const c = Scenes.Cover3D(s.querySelector(".cover-bg"));
      return { enter: c.start, leave: c.stop };
    },

    frontier(s) { return Scenes.QuadRun(s.querySelector(".quad-host")); },

    dual(s) { return Scenes.VecHeap3D(s.querySelector(".vh-host")); },

    theory(s) {
      Charts.TheoryChart(s.querySelector(".theory-chart"));
      const rows = DADOS.grafos.slice().sort((a, b) => a.n - b.n).map((g) => {
        const v = g.n * g.n * 1e-9, h = (g.n + g.m) * Math.log2(g.n + g.m) * 1e-9;
        return `<tr><td>${g.nome}</td><td>${fBig(g.n)}</td><td>${fBig(g.m)}</td><td class="c-dij">${fDur(h)}</td><td class="c-vec">${fDur(v)}</td></tr>`;
      });
      s.querySelector(".theory-table").innerHTML =
        `<thead><tr><th>grafo</th><th>n</th><th>m</th><th>heap</th><th>vetor</th></tr></thead><tbody>${rows.join("")}</tbody>`;
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
        deg.show(id, id === "rede_colaboracao" ? GIGANTES : []);
        s.querySelector(".deg-stats").innerHTML = `
          <div class="stat"><span class="v">${g.graus.minimo}</span><span class="l">grau mínimo</span></div>
          <div class="stat"><span class="v">${fInt(g.graus.maximo)}</span><span class="l">grau máximo</span></div>
          <div class="stat"><span class="v">${fNum(g.graus.mediano, 1)}</span><span class="l">mediana</span></div>
          <div class="stat"><span class="v">${fNum((100 * g.componentes.maiores[0]) / g.n, 1)}%</span><span class="l">na maior componente</span></div>`;
        s.querySelector(".deg-nota").innerHTML = id === "rede_colaboracao"
          ? `<b class="c-dfs">Os dois gigantes</b> ficam bem longe do resto: <b>Wei Wang</b> (grau 643) e <b>Wei Li</b> (605). O próximo tem 477.
             Muito provavelmente nenhum dos dois é uma pessoa só: são nomes muito comuns, e pesquisadores homônimos acabam somados num vértice. No Estudo 3 dá para usar os dois como origem.`
          : "";
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
        Cada passo do heap faz comparações em O(log(n+m)) e acumula entradas descartadas, enquanto a fila e a pilha fazem O(1).
        Nos grafos <b>sem peso</b> (todo peso = 1) a diferença é pequena: de ${fNum(d3.min(sem), 1)}× a ${fNum(d3.max(sem), 1)}×. A BFS e a DFS praticamente empatam sempre.`;
      const strip = Charts.StripChart(s.querySelector(".strip-chart"));
      function pick(id) {
        const g = G(id);
        s.querySelector(".perf-graph").value = id;
        strip.show(id);
        const b = g.benchmark;
        s.querySelector(".strip-stats").innerHTML = Charts.SERIES.map((x) =>
          `<div class="stat"><span class="v" style="color:${x.cor()}">${fMs(b[x.key].media)}</span><span class="l">${x.nome}</span></div>`).join("");
      }
      const chart = Charts.PerfBars(s.querySelector(".perf-chart"), { onSelect: pick, selected: "W_5" });
      const sel = s.querySelector(".perf-graph");
      sel.innerHTML = DADOS.grafos.map((g) => `<option value="${g.id}">${g.nome}</option>`).join("");
      sel.addEventListener("change", () => { chart.select(sel.value); pick(sel.value); });
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
      const nIn = s.querySelector(".tree-n");
      const show = () => chart.show(sel.value, alg, +nIn.value);
      nIn.addEventListener("input", () => { s.querySelector(".tree-nv").textContent = nIn.value; });
      nIn.addEventListener("change", show);
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
      sel.value = "rede_colaboracao";
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
        <div class="card"><h3>distância de 10 até cada destino</h3><div class="chart e1-chart"></div>
          <p class="note">O tamanho da barra é a <b class="c-dij">distância</b>, ou seja, a soma dos pesos do caminho mínimo. Entre parênteses está quantas arestas o caminho usa.
            O Dijkstra minimiza a soma dos pesos, não o número de arestas.</p></div></div>`;
      const chart = e1Bars(body.querySelector(".e1-chart"));
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
        if (!g.erro) chart.show(g);
      };
      body.querySelectorAll(".e1-tabs button").forEach((b) => b.addEventListener("click", () => {
        body.querySelectorAll(".e1-tabs button").forEach((x) => x.classList.toggle("on", x === b));
        show(+b.dataset.i);
      }));
      show(0);
    },

    e3(s) {
      const body = s.querySelector(".e3-body");
      const E = window.ESTUDOS && ESTUDOS.estudo3;
      if (!E) return placeholder(body, "estudo3");
      body.innerHTML = `<div class="row"><span class="mono muted" style="font-size:12px">origem</span>
          <select class="sel e3-origem">${E.origens.map((o, i) => `<option value="${i}">${o.origem}</option>`).join("")}</select>
          <div class="seg e3-tabs">${GRUPOS_E3.map((g, i) => `<button data-i="${i}" class="${i ? "" : "on"}">${g.label}</button>`).join("")}</div></div>
        <div class="grid-2-wide e3-grid"><div class="card rg-slot" style="padding:8px"></div>
        <div class="card"><p class="note e3-nota"></p><div class="e3-list"></div></div></div>`;
      const semCaminho = (d) => !d.coautores || !d.coautores.length
        ? " Não tem nenhum coautor na rede: é um vértice isolado (grau 0)."
        : ` Na rede, ${d.coautores.length > 1 ? "os coautores são" : "o único coautor é"} <b>${d.coautores.join(", ")}</b>, e ${d.coautores.length > 1 ? "eles" : "os dois"} não se ligam ao resto.`;
      let origem = 0, grupo = 0;
      function show() {
        const o = E.origens[origem];
        const g = GRUPOS_E3[grupo];
        const porNome = new Map(o.destinos.map((d) => [d.destino, d]));
        const destinos = g.nomes.map((n) => porNome.get(n)).filter(Boolean);
        s.querySelector(".e3-titulo").textContent = `De ${o.origem} a…`;
        // um grafo novo por aba: o ResearchGraph se desenha no container que recebe
        const slot = body.querySelector(".rg-slot");
        slot.innerHTML = `<div class="chart rg"></div>`;
        const host = slot.firstChild;
        if (destinos.length) Scenes.ResearchGraph(host, { origem: o.origem, destinos });
        else host.innerHTML = `<p class="note" style="padding:16px">${o.origem} é a própria origem.</p>`;
        const nota = body.querySelector(".e3-nota");
        nota.innerHTML = g.nota || "";
        nota.style.display = g.nota ? "" : "none";
        const list = body.querySelector(".e3-list");
        list.innerHTML = destinos.map((d) => `
          <div class="e3-item" data-n="${d.destino}"><div class="row"><b>${d.destino}</b><span class="spacer"></span>
            <span class="mono c-bfs">${d.distancia == null ? "∞" : fNum(d.distancia, 3)}</span></div>
            ${d.caminho ? `<span class="note">${d.caminho.length - 1} arestas</span>${pathHTML(d.caminho)}` : `<span class="note"><b class="c-danger">Sem caminho:</b> está em outra componente conexa.${semCaminho(d)}</span>`}</div>`).join("");
        list.querySelectorAll(".e3-item").forEach((it) => {
          it.addEventListener("mouseenter", () => host.highlight && host.highlight(it.dataset.n));
          it.addEventListener("mouseleave", () => host.highlight && host.highlight(null));
        });
      }
      body.querySelectorAll(".e3-tabs button").forEach((b) => b.addEventListener("click", () => {
        body.querySelectorAll(".e3-tabs button").forEach((x) => x.classList.toggle("on", x === b));
        grupo = +b.dataset.i;
        show();
      }));
      body.querySelector(".e3-origem").addEventListener("change", (e) => { origem = +e.target.value; show(); });
      show();
    },

    thanks(s) {
      const c = Scenes.Cover3D(s.querySelector(".cover-bg"));
      return { enter: c.start, leave: c.stop };
    },
  };

  // ------------------------------------------------------ auxiliares
  const FORMATOS = {
    estudo1: "Rodar dijkstra_heap(10) em cada grafo com pesos (ver Estudos/run_estudos.sh).",
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

  // Estudo 1: uma barra por destino no grafo escolhido
  function e1Bars(container) {
    const svg = d3.select(container).append("svg");
    let g = null, size = null;
    function draw(anim) {
      if (!g || !size) return;
      const [w, h] = size;
      svg.selectAll("*").remove();
      const pts = g.destinos;
      const m = { t: 8, r: 150, b: 40, l: 70 };
      const y = d3.scaleBand().domain(pts.map((d) => d.destino)).range([m.t, h - m.b]).padding(0.3);
      const x = d3.scaleLinear().domain([0, d3.max(pts, (d) => d.distancia) || 1]).nice().range([m.l, w - m.r]);
      const fim = (d) => Math.max(2, x(d.distancia ?? 0) - m.l);
      svg.append("g").attr("class", "gridline").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).ticks(5).tickSize(-(h - m.t - m.b)).tickFormat(""));
      svg.append("g").attr("class", "axis").attr("transform", `translate(0,${h - m.b})`).call(d3.axisBottom(x).ticks(5));
      svg.append("text").attr("class", "axis-label").attr("x", (m.l + w - m.r) / 2).attr("y", h - 6).attr("text-anchor", "middle").text("distância (soma dos pesos)");
      svg.append("g").attr("class", "axis").attr("transform", `translate(${m.l},0)`).call(d3.axisLeft(y).tickSize(0).tickFormat((d) => `10 → ${d}`)).select(".domain").remove();
      const row = svg.append("g").selectAll("g").data(pts).join("g").attr("transform", (d) => `translate(0,${y(d.destino)})`);
      row.append("rect").attr("x", m.l).attr("height", y.bandwidth()).attr("rx", 4).attr("fill", css("--dij"))
        .attr("width", anim ? 0 : fim).transition().duration(anim ? 800 : 0).delay((d, i) => (anim ? i * 90 : 0)).attr("width", fim);
      row.append("text").attr("x", (d) => m.l + fim(d) + 8).attr("y", y.bandwidth() / 2).attr("dy", "0.35em").attr("fill", css("--text"))
        .attr("font-size", 12).attr("font-family", "var(--font-mono)")
        .text((d) => (d.distancia == null ? "inalcançável" : `${fNum(d.distancia, 3)}  (${d.caminho.length - 1} arestas)`));
    }
    new ResizeObserver(() => {
      const r = container.getBoundingClientRect();
      if (r.width > 10 && r.height > 10) { size = [r.width, r.height]; draw(false); }
    }).observe(container);
    return { show(novo) { g = novo; draw(true); } };
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
