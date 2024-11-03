#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define TAMANHO_FRAME 4096              // 4KB, tamanho típico de página/frame
#define NUM_FRAMES 16                   // Número de frames na memória física

//estrutura de um processo
typedef struct {
    int pid;                // ID do processo
    int *enderecos;         // Array com endereços virtuais que o processo acessará
    int num_enderecos;      // Quantidade de endereços
    int tamanho_processo;   // Tamanho do processo em bytes
} Processo;

//estrutura de frame individual na memória física
typedef struct {
    int id;                 // ID do frame
    bool ocupado;           // Indica se o frame está em uso
    int processo_id;        // ID do processo que está usando este frame (-1 se livre)
    int pagina_id;          // ID da página armazenada neste frame (-1 se livre)
    char *dados;            // Ponteiro para os dados armazenados no frame
} Frame;

//estrutura para representar uma entrada na tabela de páginas
typedef struct {
    int pagina;             // Página virtual
    int frame;              // Frame físico mapeado
} EntradaTabelaPaginas;

//função que inicia processo com endereços virtuais aleatórios
Processo* inicializar_processo(int pid, int tamanho, int num_enderecos, int TamanhoMemoriaVirtual) {
    Processo *processo = (Processo*)malloc(sizeof(Processo));
    processo->pid = pid;
    processo->tamanho_processo = tamanho;
    processo->num_enderecos = num_enderecos;
    processo->enderecos = (int*)malloc(num_enderecos * sizeof(int));

    for (int i = 0; i < num_enderecos; i++) {
        processo->enderecos[i] = rand() % TamanhoMemoriaVirtual;
    }

    return processo;
}

//função para inicializar a memória física com frames livres
Frame* inicializar_memoria_fisica() {
    Frame *memoria_fisica = (Frame*)malloc(NUM_FRAMES * sizeof(Frame));
    for (int i = 0; i < NUM_FRAMES; i++) {
        memoria_fisica[i].id = i;
        memoria_fisica[i].ocupado = false;  
        memoria_fisica[i].processo_id = -1;
        memoria_fisica[i].pagina_id = -1;
        memoria_fisica[i].dados = (char*)malloc(TAMANHO_FRAME * sizeof(char));
    }
    return memoria_fisica;
}

//função para inicializar a tabela de páginas
EntradaTabelaPaginas* inicializar_tabela_paginas(int num_paginas) {
    EntradaTabelaPaginas *tabela = (EntradaTabelaPaginas*)malloc(num_paginas * sizeof(EntradaTabelaPaginas));
    for (int i = 0; i < num_paginas; i++) {
        tabela[i].pagina = i;
        tabela[i].frame = -1;
    }
    return tabela;
}

//função para alocar um frame livre para uma página
int alocar_frame(Frame *memoria_fisica) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (!memoria_fisica[i].ocupado) {
            memoria_fisica[i].ocupado = true;
            return memoria_fisica[i].id;
        }
    }
    //retorando -1 se não tem frames livres
    return -1;  
}

//função para desalocar um frame
void desalocar_frame(Frame *memoria_fisica, int frame_id) {
    if (frame_id >= 0 && frame_id < NUM_FRAMES) {
        memoria_fisica[frame_id].ocupado = false;
        memoria_fisica[frame_id].processo_id = -1;
        memoria_fisica[frame_id].pagina_id = -1;
        free(memoria_fisica[frame_id].dados);
        memoria_fisica[frame_id].dados = (char*)malloc(TAMANHO_FRAME * sizeof(char));
    }
}

//função para mapear uma página para um frame
void mapear_pagina(Frame *memoria_fisica, EntradaTabelaPaginas *tabela_paginas, int pid, int pagina) {
    int frame_id = alocar_frame(memoria_fisica);
    if (frame_id != -1) {
        memoria_fisica[frame_id].processo_id = pid;
        memoria_fisica[frame_id].pagina_id = pagina;
        tabela_paginas[pagina].frame = frame_id;
        printf("Página %d do processo %d mapeada para o frame %d\n", pagina, pid, frame_id);
    } else {
        printf("Page Fault: Não há frames livres para a página %d do processo %d\n", pagina, pid);
    }
}

//função para traduzir um endereço virtual em endereço físico
int traduzir_endereco(Processo *processo, int endereco_virtual, Frame *memoria_fisica, EntradaTabelaPaginas *tabela_paginas, int num_paginas) {
    int pagina = endereco_virtual / TAMANHO_FRAME;
    int offset = endereco_virtual % TAMANHO_FRAME;

    if (pagina >= num_paginas) {
        printf("Erro: Endereço virtual %d está fora dos limites.\n", endereco_virtual);
        return -1;
    }

    if (tabela_paginas[pagina].frame == -1) {
        printf("Page Fault: Página %d não está presente na memória física.\n", pagina);
        return -1;
    }

    int frame = tabela_paginas[pagina].frame;
    return (frame * TAMANHO_FRAME) + offset;
}

int main() {

    int TamanhoMemoriaVirtual = 65536;

    // Inicializa estruturas de dados
    Frame *memoria_fisica = inicializar_memoria_fisica();
    int num_paginas = TamanhoMemoriaVirtual / TAMANHO_FRAME;
    EntradaTabelaPaginas *tabela_paginas = inicializar_tabela_paginas(num_paginas);

    // Inicializa processo
    Processo *processo = inicializar_processo(1, 8192, 5, 65536);

    // Aloca páginas do processo para frames físicos
    printf("Alocando páginas do Processo %d:\n", processo->pid);
    for (int i = 0; i < processo->num_enderecos; i++) {
        int pagina = processo->enderecos[i] / TAMANHO_FRAME;
        mapear_pagina(memoria_fisica, tabela_paginas, processo->pid, pagina);
    }

    // Traduzir endereços virtuais para físicos
    printf("\nTradução de endereços virtuais para o Processo %d:\n", processo->pid);
    for (int i = 0; i < processo->num_enderecos; i++) {
        int endereco_fisico = traduzir_endereco(processo, processo->enderecos[i], memoria_fisica, tabela_paginas, num_paginas);
        if (endereco_fisico == -1) {
            printf("Falha de página para endereço virtual %d\n", processo->enderecos[i]);
        } else {
            printf("Endereço virtual %d traduzido para físico %d\n", processo->enderecos[i], endereco_fisico);
        }
    }

    // Libera memória alocada
    for (int i = 0; i < NUM_FRAMES; i++) {
        free(memoria_fisica[i].dados);
    }
    free(memoria_fisica);
    free(tabela_paginas);
    free(processo->enderecos);
    free(processo);
}