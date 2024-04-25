#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar strings
#include <time.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#ifdef WIN32
#include <windows.h> // inclui apenas no Windows
#include "gl/glut.h"
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

// SOIL é a biblioteca para leitura das imagens
#include "SOIL.h"

// Um pixel RGBpixel (24 bits)
typedef struct
{
    unsigned char r, g, b;
} RGBpixel;

// Uma imagem RGBpixel
typedef struct
{
    int width, height;
    RGBpixel *pixels;
} Img;

// Protótipos
void load(char *name, Img *pic);
void valida();
int cmp(const void *elem1, const void *elem2);

// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);

// Largura e altura da janela
int width, height;

#define tolerancia  50.0

// calcula a distancia euclidiana entre duas cores e retorna ok caso a distancia seja menor ou igual à tolerancia.
// usado para diferenciar se preenche com preto ou se usa a cor mais próxima disponível
bool coresProximas(RGBpixel cor1, RGBpixel cor2) {
    RGBpixel cor1_copy = cor1;
    RGBpixel cor2_copy = cor2;

    // int distancia = distanciaManhattan(cor1_copy, cor2_copy);
    // return distancia <= tolerancia;
    
    double distancia = sqrt(pow((cor1_copy.r - cor2_copy.r), 2) + pow((cor1_copy.g - cor2_copy.g), 2) + pow((cor1_copy.b - cor2_copy.b), 2));    
    return distancia <= tolerancia;
}

// Função para embaralhar os pixels de uma imagem
void embaralharImagem(RGBpixel *imagem, int largura, int altura) {
    // Define a semente para a função rand() com base no tempo atual
    srand(time(NULL));

    // Calcula o total de pixels na imagem
    int total_pixels = largura * altura;

    // Embaralha os pixels
    for (int i = 0; i < total_pixels; i++) {
        // Escolhe um índice aleatório para trocar com o índice atual
        int j = rand() % total_pixels;

        // Troca os pixels nas posições i e j
        RGBpixel temp = imagem[i];
        imagem[i] = imagem[j];
        imagem[j] = temp;
    }
}

void preencherImagem(RGBpixel *imagem_saida, RGBpixel *imagem_origem, RGBpixel *imagem_desejada, int largura_saida, int altura_saida, int largura_origem, int altura_origem, int largura_desejada, int altura_desejada) {
    // Cria um array unidimensional para marcar se um pixel de origem já foi usado
    embaralharImagem(imagem_origem, largura_origem, altura_origem);
   
    bool *pixel_usado = (bool *)malloc(largura_origem * altura_origem * sizeof(bool));
    if (pixel_usado == NULL) {
        // Verifica se a alocação de memória foi bem-sucedida
        printf("Erro ao alocar memória para o array de pixels usados\n");
        return;
    }

    int total_pixels_saida = largura_saida * altura_saida;

    // Inicializa todos os elementos desse array como não usados
    for (int i = 0; i < largura_origem * altura_origem; i++) {
        pixel_usado[i] = false;
    }

    RGBpixel *pixels_origem_usados = (RGBpixel *)malloc(total_pixels_saida * sizeof(RGBpixel));


    for (int i = 0; i < altura_saida; i++) {
        for (int j = 0; j < largura_saida; j++) {
            // Índice do pixel na imagem de saída
            int idx_saida = i * largura_saida + j;

            // Índice correspondente na imagem desejada
            int idx_desejada_i = i * altura_desejada / altura_saida;
            int idx_desejada_j = j * largura_desejada / largura_saida;
            int idx_desejada = idx_desejada_i * largura_desejada + idx_desejada_j;

            // Define a variável para dizer se encontrou a cor ou se vai preencher preto
            bool cor_encontrada = false;

            // Itera sobre todos os pixels na imagem de origem para encontrar a mais próxima
            for (int k = 0; k < altura_origem; k++) {
                for (int l = 0; l < largura_origem; l++) {
                    // Índice do pixel na imagem de origem
                    int idx_origem = k * largura_origem + l;

                    // Verifica se o pixel de origem já foi usado
                    if (!pixel_usado[idx_origem]) {
                        // Usa a função coresProximas para verificar se a cor da imagem de origem é próxima o suficiente
                        
                        if (coresProximas(imagem_desejada[idx_desejada], imagem_origem[idx_origem])) {
                            // Atribui a cor da imagem de origem à imagem de saída                           
                            pixels_origem_usados[idx_saida] = imagem_saida[idx_saida];
                            imagem_saida[idx_saida] = imagem_origem[idx_origem];

                            // Marca o pixel de origem como usado
                            cor_encontrada = true;
                            pixel_usado[idx_origem] = true;
                            break; // Sai do loop interno se encontrar uma cor próxima

                        }else
                        {
                            imagem_saida[idx_saida] = imagem_origem[idx_origem];
                        }
                    }
                }
                if (cor_encontrada) {
                    break; // Sai do loop externo se encontrar uma cor próxima
                }
            }   
            // Se nenhuma cor próxima foi encontrada, preenche o pixel com a cor da imagem de origem
            if (!cor_encontrada) {
                imagem_saida[idx_saida] = pixels_origem_usados[idx_saida];

                //printf("r: %d , g: %d, b: %d \n", imagem_origem[idx_saida].r, imagem_origem[idx_saida].g, imagem_origem[idx_saida].b);
            }     
        }
    }
    // Libera a memória alocada para o array de pixels usados
    free(pixel_usado);
}

// Compara se a troca é vantajoso para ambas as posições
int compararPixel(RGBpixel *ref1, RGBpixel *ref2, RGBpixel *pixel1, RGBpixel *pixel2) {
    int valueP1Red = abs(ref1->r - pixel1->r);
    int valueP1Blue = abs(ref1->b - pixel1->b);
    int valueP1Green = abs(ref1->g - pixel1->g);
    int difRef1P1 = valueP1Red + valueP1Blue + valueP1Green;

    int valueP2Red = abs(ref1->r - pixel2->r);
    int valueP2Blue = abs(ref1->b - pixel2->b);
    int valueP2Green = abs(ref1->g - pixel2->g);
    int difRef1P2 = valueP2Red + valueP2Blue + valueP2Green;

    int ref2valueP1Red = abs(ref2->r - pixel1->r);
    int ref2valueP1Blue = abs(ref2->b - pixel1->b);
    int ref2valueP1Green = abs(ref2->g - pixel1->g);
    int difRef2P1 = ref2valueP1Red + ref2valueP1Blue + ref2valueP1Green;

    int ref2valueP2Red = abs(ref2->r - pixel2->r);
    int ref2valueP2Blue = abs(ref2->b - pixel2->b);
    int ref2valueP2Green = abs(ref2->g - pixel2->g);
    int difRef2P2 = ref2valueP2Red + ref2valueP2Blue + ref2valueP2Green;

    if (difRef1P2 < difRef1P1 && difRef2P1 < difRef2P2) {
        return 1;
    }

    return 0;
}


// Identificadores de textura
GLuint tex[3];

// As 3 imagens
Img pic[3];

// Imagem selecionada (0,1,2)
int sel;

// Enums para facilitar o acesso às imagens
#define ORIGEM 0
#define DESEJ 1
#define SAIDA 2

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("alchemy [origem] [destino]\n");
        printf("Origem é a fonte das cores, destino é a imagem desejada\n");
        exit(1);
    }
    glutInit(&argc, argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // pic[0] -> imagem com as cores
    // pic[1] -> imagem desejada
    // pic[2] -> resultado do algoritmo

    // Carrega as duas imagens
    load(argv[1], &pic[ORIGEM]);
    load(argv[2], &pic[DESEJ]);

    // A largura e altura da janela são calculadas de acordo com a maior
    // dimensão de cada imagem
    width = pic[ORIGEM].width > pic[DESEJ].width ? pic[ORIGEM].width : pic[DESEJ].width;
    height = pic[ORIGEM].height > pic[DESEJ].height ? pic[ORIGEM].height : pic[DESEJ].height;

    // A largura e altura da imagem de saída são iguais às da imagem desejada (1)
    pic[SAIDA].width = pic[DESEJ].width;
    pic[SAIDA].height = pic[DESEJ].height;
    pic[SAIDA].pixels = malloc(pic[DESEJ].width * pic[DESEJ].height * 3); // W x H x 3 bytes (RGBpixel)

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(width, height);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Quebra-Cabeca digital");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(draw);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc(keyboard);

    // Cria texturas em memória a partir dos pixels das imagens
    tex[ORIGEM] = SOIL_create_OGL_texture((unsigned char *)pic[ORIGEM].pixels, pic[ORIGEM].width, pic[ORIGEM].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[DESEJ] = SOIL_create_OGL_texture((unsigned char *)pic[DESEJ].pixels, pic[DESEJ].width, pic[DESEJ].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Exibe as dimensões na tela, para conferência
    printf("Origem   : %s %d x %d\n", argv[1], pic[ORIGEM].width, pic[ORIGEM].height);
    printf("Desejada : %s %d x %d\n", argv[2], pic[DESEJ].width, pic[DESEJ].height);
    sel = ORIGEM; // pic1

    // Define a janela de visualizacao 2D
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, width, height, 0.0);
    glMatrixMode(GL_MODELVIEW);

    srand(time(0)); // Inicializa gerador aleatório (se for usar random)

    printf("Processando...\n");

    // Copia imagem de origem na imagem de saída
    // (NUNCA ALTERAR AS IMAGENS DE ORIGEM E DESEJADA)
    int tam = pic[ORIGEM].width * pic[ORIGEM].height;
    memcpy(pic[SAIDA].pixels, pic[ORIGEM].pixels, sizeof(RGBpixel) * tam);

    //
    // Neste ponto, voce deve implementar o algoritmo!
    // (ou chamar funcoes para fazer isso)
    //
    // Aplica o algoritmo e gera a saida em pic[SAIDA].pixels...
    // ...
    // ...
    //
    // Exemplo de manipulação: inverte as cores na imagem de saída
    /**/
    // Implementação do algoritmo de transmutação de imagens

    //Inicialização de variáveis para mostrar o progresso no console
    int totaliteracoes = pic[SAIDA].height * pic[SAIDA].width;
    int progresso = 0;
    
    preencherImagem(pic[SAIDA].pixels, pic[ORIGEM].pixels, pic[DESEJ].pixels, pic[SAIDA].width, pic[SAIDA].height, pic[ORIGEM].width, pic[ORIGEM].height,
     pic[DESEJ].width, pic[DESEJ].height);

     valida();

    /**/

    // NÃO ALTERAR A PARTIR DAQUI!

    // Cria textura para a imagem de saída
    tex[SAIDA] = SOIL_create_OGL_texture((unsigned char *)pic[SAIDA].pixels, pic[SAIDA].width, pic[SAIDA].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    // Grava imagem de saída em out.bmp, para conferência
    SOIL_save_image("out.bmp", SOIL_SAVE_TYPE_BMP, pic[SAIDA].width, pic[SAIDA].height, 3, (const unsigned char *)pic[SAIDA].pixels);

    // Entra no loop de eventos, não retorna
    glutMainLoop();
}

// Carrega uma imagem para a struct Img
void load(char *name, Img *pic)
{
    int chan;
    pic->pixels = (RGBpixel *)SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if (!pic->pixels)
    {
        printf("SOIL loading error: '%s'\n", SOIL_last_result());
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

// Verifica se o algoritmo foi aplicado corretamente:
// Ordena os pixels da imagem origem e de saída por R, G e B;
// depois compara uma com a outra: devem ser iguais
void valida()
{
    int ok = 1;
    int size = pic[ORIGEM].width * pic[ORIGEM].height;
    // Aloca memória para os dois arrays
    RGBpixel *aux1 = malloc(size * 3);
    RGBpixel *aux2 = malloc(size * 3);
    // Copia os pixels originais
    memcpy(aux1, pic[ORIGEM].pixels, size * 3);
    memcpy(aux2, pic[SAIDA].pixels, size * 3);
    // Mostra primeiros 8 pixels de ambas as imagens
    // antes de ordenar (teste)
    for (int i = 0; i < 8; i++)
        printf("[%d %d %d] ", aux1[i].r, aux1[i].g, aux1[i].b);
    printf("\n");
    for (int i = 0; i < 8; i++)
        printf("[%d %d %d] ", aux2[i].r, aux2[i].g, aux2[i].b);
    printf("\n");
    printf("Validando...\n");
    // Ordena ambos os arrays
    qsort(aux1, size, sizeof(RGBpixel), cmp);
    qsort(aux2, size, sizeof(RGBpixel), cmp);
    // Mostra primeiros 8 pixels de ambas as imagens
    // depois de ordenar
    for (int i = 0; i < 8; i++)
        printf("[%d %d %d] ", aux1[i].r, aux1[i].g, aux1[i].b);
    printf("\n");
    for (int i = 0; i < 8; i++)
        printf("[%d %d %d] ", aux2[i].r, aux2[i].g, aux2[i].b);
    printf("\n");
    for (int i = 0; i < size; i++)
    {
        if (aux1[i].r != aux2[i].r ||
            aux1[i].g != aux2[i].g ||
            aux1[i].b != aux2[i].b)
        {
            // Se pelo menos um dos pixels for diferente, o algoritmo foi aplicado incorretamente
            printf("*** INVALIDO na posicao %d ***: %02X %02X %02X -> %02X %02X %02X\n",
                   i, aux1[i].r, aux1[i].g, aux1[i].b, aux2[i].r, aux2[i].g, aux2[i].b);
            ok = 0;
            break;
        }
    }
    // Libera memória dos arrays ordenados
    free(aux1);
    free(aux2);
    if (ok)
        printf(">>>> TRANSFORMACAO VALIDA <<<<<\n");
}

// Funcao de comparacao para qsort: ordena por R, G, B (desempate nessa ordem)
int cmp(const void *elem1, const void *elem2)
{
    RGBpixel *ptr1 = (RGBpixel *)elem1;
    RGBpixel *ptr2 = (RGBpixel *)elem2;
    unsigned char r1 = ptr1->r;
    unsigned char r2 = ptr2->r;
    unsigned char g1 = ptr1->g;
    unsigned char g2 = ptr2->g;
    unsigned char b1 = ptr1->b;
    unsigned char b2 = ptr2->b;
    int r = 0;
    if (r1 < r2)
        r = -1;
    else if (r1 > r2)
        r = 1;
    else if (g1 < g2)
        r = -1;
    else if (g1 > g2)
        r = 1;
    else if (b1 < b2)
        r = -1;
    else if (b1 > b2)
        r = 1;
    return r;
}

//
// Funções de callback da OpenGL
//
// SÓ ALTERE SE VOCÊ TIVER ABSOLUTA CERTEZA DO QUE ESTÁ FAZENDO!
//

// Gerencia eventos de teclado
void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        // ESC: libera memória e finaliza
        free(pic[0].pixels);
        free(pic[1].pixels);
        free(pic[2].pixels);
        exit(1);
    }
    if (key >= '1' && key <= '3')
        // 1-3: seleciona a imagem correspondente (origem, desejada e saída)
        sel = key - '1';
    // V para validar a solução
    if (key == 'v')
        valida();
    glutPostRedisplay();
}

// Callback de redesenho da tela
void draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Preto
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/rgb.txt

    glColor3ub(255, 255, 255); // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0, 0);
    glVertex2f(0, 0);

    glTexCoord2f(1, 0);
    glVertex2f(pic[sel].width, 0);

    glTexCoord2f(1, 1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0, 1);
    glVertex2f(0, pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
