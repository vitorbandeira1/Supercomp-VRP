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

using namespace std;

using Local = int;
using Carga = int;
using Custo = int;

struct Caminho {
    vector<Local> trajeto;
    Custo custoTotal;

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

int main() {
    // vector<string> nomesArquivos = {
    //     "../grafos/ingrafo1.txt",
    //     "../grafos/ingrafo2.txt",
    //     "../grafos/ingrafo3.txt",
    //     "../grafos/ingrafo4.txt",
    //     "../grafos/ingrafo5.txt",
    //     "../grafos/ingrafo6.txt",
    //     "../grafos/ingrafo7.txt",
    // };
    vector<string> nomesArquivos = {
        "../grafos/ingrafo1.txt",
        "../grafos/ingrafo2.txt",
        "../grafos/grafo_6.txt",
        "../grafos/grafo_7.txt",
        "../grafos/grafo_8.txt",
        "../grafos/grafo_9.txt",
        "../grafos/grafo_10.txt",
    };

    for (const auto& nomeArquivo : nomesArquivos) {
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

        for (int i = 0; i < numLocais; ++i) {
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
        auto tempoFim = chrono::high_resolution_clock::now();
        auto duracao = chrono::duration_cast<chrono::milliseconds>(tempoFim - tempoInicio).count();
        double duracaoSegundos = duracao / 1000.0;

        cout << "Solucao para: " << nomeArquivo << endl;
        cout << "Sequencia de locais na melhor rota: ";
        for (const Local& local : melhorCaminho.trajeto) cout << local << " -> ";
        cout << "0" << endl;
        cout << "Custo da melhor rota: " << melhorCaminho.custoTotal << endl;
        cout << "Tempo de exec: " << duracao << " milissegundos (" << fixed << setprecision(3) << duracaoSegundos << " segundos)." << endl;
        cout << "--------------------------------------------------------" << endl;
    }
}
