#include <avr/io.h>
#include <avr/interrupt.h>
#include <LiquidCrystal.h>
#include <string.h>

LiquidCrystal lcd(8, 9, 10, 11, 12, 13);
#define TIMER1_MODULE 15625
#define FALSE 0
#define TRUE 1
#define _ltrans(x, xi, xm, yi, ym) (long)((long)(x - xi) * (long)(ym - yi)) / (long)(xm - xi) + yi
#define NORMAL 0
#define CRON_START 1
#define CRON_PAUSE 2
#define CRON_STOP 3
#define ALARM_SEG 4
#define ALARM_MIN 5
#define ALARM_HRA 6
#define REL_SEG 7
#define REL_MIN 8
#define REL_HRA 9
#define ALARME_ATIV 10

typedef struct
{
    char hora;
    char minuto;
    char segundo;
    char segundoAnt;
} Rel;

typedef enum
{
    HORA_ATUAL,
    CRONOMETRO,
    ALARME_REL,
    ACERTA_REL

} relogio_state;

unsigned int Le_AD(void);
void AcertaRelogio(Rel *r);
void configura_alarme(Rel *a);
void print_hora_atual(Rel *r, Rel *A);
void conta_cronometro(Rel *r);

relogio_state Estado_Rel = HORA_ATUAL;
char Seleciona = FALSE;
int counter = 0;
Rel *pts;

char *mensagem[20] = {
    "   Hora Atual   ",
    "Crono - Iniciar ",
    " Crono - Pausar ",
    " Crono - Parar  ",
    "Set Alarme - Seg",
    "Set Alarme - Min",
    "Set Alarme - Hra",
    " Set Rel - Seg  ",
    " Set Rel - Min  ",
    " Set Rel - Hra  ",
    "   ALARME !!!!  "};

int index = 0;

int main(void)
{
    Rel Relogio = {0, 0, 0, 0};
    Rel Alarme = {9, 9, 9, 9};
    pts = &Relogio;
    init_dsp(2, 16);
    putmessage(0, 3, mensagem[1]);
    putmessage(1, 4, "00:00:00");

    OCR1A = TIMER1_MODULE - 1;
    TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10); // FPS = 1024
    TIMSK1 = _BV(OCIE1A);
    EICRA = _BV(ISC01) | _BV(ISC00);
    EIMSK = _BV(INT0) | _BV(INT1);
    sei();

    for (;;)
    {
        putmessage(0, 0, mensagem[index]);
        switch (Estado_Rel)
        {
        case HORA_ATUAL:
            print_hora_atual(&Relogio, &Alarme);
            break;

        case ALARME_REL:
            configura_alarme(&Alarme);
            break;

        case CRONOMETRO:
            conta_cronometro(&Relogio);
            break;

        case ACERTA_REL:
            AcertaRelogio(&Relogio);
            break;
        default:
            break;
        }
    }
}

void AcertaRelogio(Rel *r)
{
    static char State = -1;

    if (Seleciona)
    {
        Seleciona = FALSE;
        State++;
        State %= 4;
    }
    switch (State)
    {
    case 0:
        index = REL_HRA;
        r->hora = _ltrans(Le_AD(0), 0, 1023, 0, 23);
        putnumber_i(1, 4, r->hora, 2);
        break;
    case 1:
        index = REL_MIN;
        r->minuto = _ltrans(Le_AD(0), 0, 1023, 0, 59);
        putnumber_i(1, 7, r->minuto, 2);
        break;
    case 2:
        index = REL_SEG;
        r->segundo = _ltrans(Le_AD(0), 0, 1023, 0, 59);
        putnumber_i(1, 10, r->segundo, 2);
        break;
    case 3:
        index = NORMAL;
        Estado_Rel = HORA_ATUAL;
        break;
    }
}

void configura_alarme(Rel *a)
{
    static char State = -1;

    if (Seleciona)
    {
        Seleciona = FALSE;
        State++;
        State %= 4;
    }
    switch (State)
    {
    case 0:
        index = ALARM_HRA;
        a->hora = _ltrans(Le_AD(0), 0, 1023, 0, 23);
        putnumber_i(1, 4, a->hora, 2);
        break;
    case 1:
        index = ALARM_MIN;
        a->minuto = _ltrans(Le_AD(0), 0, 1023, 0, 59);
        putnumber_i(1, 7, a->minuto, 2);
        break;
    case 2:
        index = ALARM_SEG;
        a->segundo = _ltrans(Le_AD(0), 0, 1023, 0, 59);
        putnumber_i(1, 10, a->segundo, 2);
        break;
    case 3:
        Estado_Rel = HORA_ATUAL;
        break;
    }
}

void print_hora_atual(Rel *r, Rel *A)
{

    if (A->hora == r->hora && A->minuto == r->minuto)
    {
        index = ALARME_ATIV;
    }
    else
    {
        index = NORMAL;
    }
    if (r->segundoAnt != r->segundo)
    {
        r->segundoAnt = r->segundo;
        putnumber_i(1, 4, r->hora, 2);
        putnumber_i(1, 7, r->minuto, 2);
        putnumber_i(1, 10, r->segundo, 2);
    }
}

void conta_cronometro(Rel *r)
{
    static char State = -1;

    if (Seleciona)
    {
        Seleciona = FALSE;
        State++;
        State %= 3;
    }
    switch (State)
    {
    case 0:
        index = CRON_START;
        break;
    case 1:
        index = CRON_PAUSE;
        if (r->segundoAnt != r->segundo)
        {
            r->segundoAnt = r->segundo;
            putnumber_i(1, 4, r->hora, 2);
            putnumber_i(1, 7, r->minuto, 2);
            putnumber_i(1, 10, r->segundo, 2);
        }
    case 3:
        index = NORMAL;
        Estado_Rel = HORA_ATUAL;
        break;

    default:
        break;
    }
}

ISR(INT0_vect) // Seleciona o Modo
{

    if (Estado_Rel == HORA_ATUAL)
    {
        index = CRON_START;
        Estado_Rel = CRONOMETRO;
    }
    else if (Estado_Rel == CRONOMETRO)
    {
        index = ALARM_HRA;
        Estado_Rel = ALARME_REL;
    }
    else if (Estado_Rel == ALARME_REL)
    {
        index = REL_HRA;
        Estado_Rel = ACERTA_REL;
    }
    else if (Estado_Rel == ACERTA_REL)
    {
        index = NORMAL;
        Estado_Rel = HORA_ATUAL;
    }
}

ISR(INT1_vect) //Seleciona o modo
{
    Seleciona = TRUE;
}

ISR(TIMER1_COMPA_vect)
{
    if (!Estado_Rel)
    {
        if (++pts->segundo == 60)
        {
            pts->segundo = 0;
            if (++pts->minuto == 60)
            {
                pts->minuto = 0;
                if (++pts->hora == 24)
                    pts->hora = 0;
            }
        }
    }
}

unsigned int Le_AD(char channel)
{
    static char FirstTime = 1;

    if (FirstTime)
    {
        FirstTime = 0;
        ADMUX = _BV(REFS0);                                        // Seleciona Vref igual a 5V
        ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // AD e preesc. de 128
    }
    DIDR0 = _BV(channel); // Liga a entrada analogica no pino PC0 e desliga a funcao digital
    ADMUX &= 0xF0;
    ADMUX |= channel;
    ADCSRA |= _BV(ADSC); // dispara ao conversor

    while (!(ADCSRA & _BV(ADIF))) // espera finalizar a conversao
        continue;
    return (ADC); //devolve o resultado da conversao
}

/*******    PARA USO DO DISPLAY    ***********************/
void init_dsp(int l, int c)
{
    lcd.begin(c, l);
}

void putmessage(int l, int c, char *msg)
{
    lcd.setCursor(c, l);
    lcd.print(msg);
}

void putnumber_i(int l, int c, long ni, int nd)
{
    char zero[] = {"0000000000000000"};
    long int nx;
    int i, j;

    nx = ni;
    for (i = 10, j = 1; nx > 9; i *= 10, j++)
        nx = ni / i;
    lcd.setCursor(c, l);
    lcd.print(&zero[16 - nd + j]);
    lcd.print(ni);
}

void putnumber_f(int l, int c, float ni, int nd)
{
    lcd.setCursor(c, l);
    lcd.print(ni, nd);
}
