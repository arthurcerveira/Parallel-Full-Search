# Algoritmo Full Search implementado com OpenMP

Arthur Cerveira e Raquel Zimmer

## Instruções

Para compilar e executar o algoritmo, é necessário executar os comandos:

```bash
$ mpic++ fullSearch.cpp -fopenmp -o fullSearch -lpthread
$ mpirun -np 3 ./fullSearch
```

O resultado será o vídeo codificado `coded_video.bin`, com o quadro inicial do vídeo e os vetores Ra e Rv dos quadros seguintes. A execução do programa também cria um diretório com arquivos de texto descrevendo os valores de Rv e Ra para cada quadro codificado.