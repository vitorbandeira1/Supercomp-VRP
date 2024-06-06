#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <climits>
#include <mpi.h>

using namespace std;

using Local = int;
using Carga = int;
using Custo = int;

struct Caminho {
    vector<Local> trajeto;
    Custo custoTotal;

    Caminho() : trajeto(), custoTotal(INT_MAX) {}

    Caminho(vector<Local> trajeto, Custo custoTotal)
        : trajeto(move(trajeto)), custoTotal(custoTotal) {}
};

class OtimizadorDeRota {
public:
    Caminho melhorCaminho = Caminho({}, INT_MAX);

    OtimizadorDeRota(int totalLocais, int capacidade, map<Local, map<Local, Custo>> conexoes, map<Local, Carga>& demandas)
        : totalLocais(totalLocais), capacidade(capacidade), conexoes(move(conexoes)), demandas(demandas) {}

    void calcularMelhorRota() {
        set<Local> visitados;
        visitados.insert(0);
        Caminho caminhoInicial({0}, 0);
        construirRota(visitados, 0, 0, caminhoInicial);
        melhorCaminho = caminhoInicial;
    }

private:
    int totalLocais, capacidade;
    map<Local, map<Local, Custo>> conexoes;
    map<Local, Carga>& demandas;

    void construirRota(set<Local>& visitados, Carga cargaAtual, Local ultimoLocal, Caminho& caminhoAtual) {
        while (visitados.size() < totalLocais) {
            Local proxLocal = -1;
            Custo menorCusto = INT_MAX;

            for (const auto& demanda : demandas) {
                Local local = demanda.first;
                if (visitados.find(local) == visitados.end() && conexoes[ultimoLocal].find(local) != conexoes[ultimoLocal].end()) {
                    if (cargaAtual + demandas[local] <= capacidade && conexoes[ultimoLocal][local] < menorCusto) {
                        proxLocal = local;
                        menorCusto = conexoes[ultimoLocal][local];
                    }
                }
            }

            if (proxLocal == -1) {
                caminhoAtual.trajeto.push_back(0);
                caminhoAtual.custoTotal += conexoes[ultimoLocal][0];
                ultimoLocal = 0;
                cargaAtual = 0;
            } else {
                caminhoAtual.trajeto.push_back(proxLocal);
                caminhoAtual.custoTotal += menorCusto;
                visitados.insert(proxLocal);
                cargaAtual += demandas[proxLocal];
                ultimoLocal = proxLocal;
            }
        }

        if (ultimoLocal != 0) {
            caminhoAtual.trajeto.push_back(0);
            caminhoAtual.custoTotal += conexoes[ultimoLocal][0];
        }
    }
};

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    vector<string> nomesArquivos = {
        "../grafos/ingrafo1.txt",
        "../grafos/ingrafo2.txt",
        "../grafos/grafo_6.txt",
        "../grafos/grafo_7.txt",
        "../grafos/grafo_8.txt",
        "../grafos/grafo_9.txt",
        "../grafos/grafo_10.txt",
    };

    // Dividir arquivos entre processos
    int numArquivos = nomesArquivos.size();
    int arquivosPorProcesso = numArquivos / size;
    int inicio = rank * arquivosPorProcesso;
    int fim = (rank == size - 1) ? numArquivos : inicio + arquivosPorProcesso;

    Caminho melhorCaminhoGlobal;

    for (int i = inicio; i < fim; ++i) {
        const auto& nomeArquivo = nomesArquivos[i];
        auto tempoInicio = chrono::high_resolution_clock::now();

        ifstream arquivo(nomeArquivo);
        if (!arquivo.is_open()) {
            cerr << "Erro ao abrir arquivo: " << nomeArquivo << endl;
            continue;
        }

        string linha;
        getline(arquivo, linha);
        int numLocais = stoi(linha);

        map<Local, Carga> demandasLocais;
        demandasLocais[0] = 0;

        for (int j = 0; j < numLocais; ++j) {
            getline(arquivo, linha);
            istringstream iss(linha);
            int local;
            Carga demanda;
            iss >> local >> demanda;
            demandasLocais[local] = demanda;
        }

        numLocais++;

        getline(arquivo, linha);
        int numVias = stoi(linha);
        map<Local, map<Local, Custo>> vias;

        for (int viaId = 0; viaId < numVias; ++viaId) {
            getline(arquivo, linha);
            istringstream iss(linha);
            Local origem, destino;
            Custo custo;
            iss >> origem >> destino >> custo;
            vias[origem][destino] = custo;
        }

        Carga capacidadeVeiculo = 20;

        OtimizadorDeRota CVRP(numLocais, capacidadeVeiculo, vias, demandasLocais);
        CVRP.calcularMelhorRota();

        Caminho melhorCaminho = CVRP.melhorCaminho;

        if (melhorCaminho.custoTotal < melhorCaminhoGlobal.custoTotal) {
            melhorCaminhoGlobal = melhorCaminho;
        }

        auto tempoFim = chrono::high_resolution_clock::now();
        auto duracao = chrono::duration_cast<chrono::milliseconds>(tempoFim - tempoInicio).count();
        double duracaoSegundos = duracao / 1000.0;

        cout << "Processo " << rank << " - Solucao para: " << nomeArquivo << endl;
        cout << "Sequencia de locais na melhor rota: ";
        for (const Local& local : melhorCaminho.trajeto) cout << local << " -> ";
        cout << "0" << endl;
        cout << "Custo da melhor rota: " << melhorCaminho.custoTotal << endl;
        cout << "Tempo de exec: " << duracao << " milissegundos (" << fixed << setprecision(3) << duracaoSegundos << " segundos)." << endl;
        cout << "--------------------------------------------------------" << endl;
    }

    Caminho melhorCaminhoFinal;

    MPI_Reduce(&melhorCaminhoGlobal.custoTotal, &melhorCaminhoFinal.custoTotal, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        cout << "Melhor custo total encontrado: " << melhorCaminhoFinal.custoTotal << endl;
    }

    MPI_Finalize();
    return 0;
}
