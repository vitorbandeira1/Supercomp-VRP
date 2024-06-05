// CODIGO NAO TESTADO

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
#include <omp.h>

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

    OtimizadorDeRota(int totalLocais, int capacidade, int maxLocais, map<Local, map<Local, Custo>> conexoes, map<Local, Carga>& demandas)
        : totalLocais(totalLocais), capacidade(capacidade), maxLocais(maxLocais), conexoes(move(conexoes)), demandas(demandas) {}

    void calcularMelhorRota() {
        #pragma omp parallel
        {
            set<Local> visitados;
            visitados.insert(0);
            Caminho caminhoInicial({0}, 0);

            #pragma omp single nowait
            {
                explorarRotas(visitados, 0, 0, 0, caminhoInicial);
            }
        }

        // Paralelização do cálculo das melhores rotas
        #pragma omp parallel for
        for (size_t i = 0; i < caminhos.size(); ++i) {
            #pragma omp critical
            {
                if (caminhos[i].custoTotal < melhorCaminho.custoTotal)
                    melhorCaminho = caminhos[i];
            }
        }
    }

private:
    int totalLocais, capacidade, maxLocais;
    map<Local, map<Local, Custo>> conexoes;
    vector<Caminho> caminhos;
    map<Local, Carga>& demandas;

    void explorarRotas(set<Local> visitados, int locaisVisitados, Local ultimoLocal, Carga cargaAtual, Caminho caminhoAtual) {
        for (const auto& demanda : demandas) {
            Local proxLocal = demanda.first;
            if (proxLocal == ultimoLocal || conexoes[ultimoLocal].find(proxLocal) == conexoes[ultimoLocal].end() || (visitados.find(proxLocal) != visitados.end() && proxLocal != 0))
                continue;

            bool excedeCarga = (cargaAtual + demanda.second) > capacidade;
            bool excedeLocais = (locaisVisitados + 1) > maxLocais;
            if (proxLocal != 0 && (excedeCarga || excedeLocais))
                continue;

            caminhoAtual.trajeto.push_back(proxLocal);
            caminhoAtual.custoTotal += conexoes[ultimoLocal][proxLocal];
            visitados.insert(proxLocal);

            if (proxLocal == 0) {
                if (visitados.size() == totalLocais)
                    #pragma omp critical
                    caminhos.push_back(caminhoAtual);
                #pragma omp task
                explorarRotas(visitados, 0, proxLocal, 0, caminhoAtual);
            } else {
                #pragma omp task
                explorarRotas(visitados, locaisVisitados + 1, proxLocal, cargaAtual + demanda.second, caminhoAtual);
            }

            caminhoAtual.trajeto.pop_back();
            caminhoAtual.custoTotal -= conexoes[ultimoLocal][proxLocal];
            visitados.erase(proxLocal);
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
        int maxLocaisPorRota = 3;

        OtimizadorDeRota CVRP(numLocais, capacidadeVeiculo, maxLocaisPorRota, vias, demandasLocais);
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
