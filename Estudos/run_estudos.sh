#!/bin/sh
# Roda os estudos de caso. Resultados em Dados/Estudos/*.json, logs em *.log.
cd "$(dirname "$0")"
D=~/Downloads
R=../Dados/Entrada/rede
OUT=../Dados/Estudos
mkdir -p $OUT

# Estudos 1 e 3: caminhos (rapido)
for g in W_3 W_4 W_5 W_6 W_7; do
  gzip -dc $D/grafo_$g.txt.gz | ./estudos.exe caminhos --nome $g --origem 10 --destinos 20,30,40,50,60 \
    > $OUT/caminhos_$g.json 2> $OUT/caminhos_$g.log
done
# Indices em vez de nomes: no Windows o argv estraga acentos ("Éva Tardos").
# 2722 = Edsger W. Dijkstra; os 5 primeiros destinos sao os do enunciado, o resto
# (Wei Wang, Bellman, Floyd, Warshall, Euler, Knuth, ..., professores da UFRJ) e extra.
./estudos.exe caminhos --nome rede_colaboracao --nomes $R/rede_colaboracao_vertices.txt \
  --origem 2722 \
  --destinos 11365,471365,5709,11386,343930,268534,462511,3501,174398,63586,5467,11649,2998,5884,10174,161092,82470,539065,1372,684520,699146,7040,330188,496285,309497,448684,12770,11492,392261,7346,3284,576970 \
  < $R/rede_colaboracao.txt > $OUT/caminhos_rede.json 2> $OUT/caminhos_rede.log
echo "caminhos ok" > $OUT/status.txt

# Estudo 2: tempos (lento). Vetor so ate n = 1 milhao (O(n^2)).
for g in W_3 W_4 W_5 W_6 W_7; do
  gzip -dc $D/grafo_$g.txt.gz | ./estudos.exe tempos --nome $g --k 100 --orcamento 600 \
    --orcamento-vetor 900 --max-n-vetor 1000000 > $OUT/tempos_$g.json 2> $OUT/tempos_$g.log
  echo "tempos $g ok" >> $OUT/status.txt
done
echo "tudo ok" >> $OUT/status.txt
