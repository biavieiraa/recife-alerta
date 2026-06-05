// --- CONFIGURAÇÕES DO FAROL RGB ---
const int FAROL_R = 8;
const int FAROL_G = 9;
const int FAROL_B = 10;

// --- CONFIGURAÇÕES DE HARDWARE ---
const int NUM_JOGADORES = 6; 

struct Controle {
  int b1; int id1;
  int b2; int id2;
  int b3; int id3;
  int b4; int id4;
  int b5; int id5;
  int pinoVibracao; 
};

// Mapeamento dos fios e pinos dos motores de vibração 
Controle controles[NUM_JOGADORES] = {
  { 22, 1, 23, 2, 24, 3, 25, 4, 26, 5,   6 }, // J1
  { 27, 0, 28, 2, 29, 3, 30, 4, 31, 5,   5 }, // J2
  { 32, 0, 33, 1, 34, 3, 35, 4, 36, 5,   4 }, // J3
  { 37, 0, 38, 1, 39, 2, 40, 4, 41, 5,   3 }, // J4
  { 42, 0, 43, 1, 44, 2, 45, 3, 46, 5,   2 }, // J5
  { 47, 0, 48, 1, 49, 2, 50, 3, 51, 4,   7 }  // J6 
};

// --- VARIÁVEIS DO JOGO ---
enum Papel { INOCENTE, DETETIVE, ASSASSINO };
enum FaseJogo { SORTEIO, NOITE, RESOLUCAO_NOITE, AMANHECER, VITORIA_ASSASSINO, VITORIA_INOCENTES };

Papel papeis[NUM_JOGADORES];
bool vivo[NUM_JOGADORES];
FaseJogo faseAtual = SORTEIO;

int idAssassino;
int idDetetive;
int mortoAssassino = -1;
int mortoVotacao = -1;

// Variáveis para a votação simultânea
int alvoEscolhido[NUM_JOGADORES];
bool jaVotou[NUM_JOGADORES];

void mudarFarol(int r, int g, int b);
void oscilarFarolNoite(); // Nova função para o efeito pisca/oscila da noite
void vibrarFeedback(int idJogador, int vezes, int tempo); 
int checarCliqueControle(int idJogador);
void checarVitoria();
void iniciarPartida();
void acaoNoite();
void resolucaoNoite();
void amanhecer();

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A0));

  pinMode(FAROL_R, OUTPUT);
