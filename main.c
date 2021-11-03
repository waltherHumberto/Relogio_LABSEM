#include <avr/io.h>
#include <avr/interrupt.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

#define TIMER1_MODULE 15625
#define FALSE 0
#define TRUE 1
#define _ltrans(x, xi, xm, yi, ym) (long)((long)(x - xi) * (long)(ym - yi)) / (long)(xm - xi) + yi

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
    ALARME_REL

} relogio_state;

unsigned int Le_AD(void);
void AcertaRelogio(Rel *r);
void configura_alarme(Rel *a);
void print_hora_atual(Rel *r, Rel *A);
void conta_cronometro(Rel *r);

relogio_state Estado_Rel = HORA_ATUAL;
char Seleciona = FALSE;
Rel *pts;

char *mensagem[4] = {"Hora Certa  ",
                     "Setar Alarme",
                     "Cronometro  ",
                     "Alterar HR  "};

int main(void)
{
    Rel Relogio = {0, 0, 0, 0};
    Rel Alarme = {0, 0, 0, 0};
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
    int index = 0;
    for (;;)
    {
        putmessage(0, 3, mensagem[index]);

        switch (Estado_Rel)
        {
        case HORA_ATUAL:
            print_hora_atual(&Relogio, &Alarme);
            index = 0;
            break;

        case ALARME_REL:
            configura_alarme(&Alarme);
            index = 1;
            break;

        case CRONOMETRO:
            conta_cronometro(&Relogio);
            index = 2;
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
        r->hora = _ltrans(Le_AD(0), 0, 1023, 0, 23);
        putnumber_i(1, 4, r->hora, 2);
        break;
    case 1:
        r->minuto = _ltrans(Le_AD(0), 0, 1023, 0, 59);
        putnumber_i(1, 7, r->minuto, 2);
        break;
    case 2:
        r->segundo = _ltrans(Le_AD(0), 0, 1023, 0, 59);
        putnumber_i(1, 10, r->segundo, 2);
        break;
    case 3:
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
        a->hora = _ltrans(Le_AD(0), 0, 1023, 0, 23);
        putnumber_i(1, 4, a->hora, 2);
        break;
    case 1:
        a->minuto = _ltrans(Le_AD(0), 0, 1023, 0, 59);
        putnumber_i(1, 7, a->minuto, 2);
        break;
    case 2:
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

    if (r->segundoAnt != r->segundo)
    {
        r->segundoAnt = r->segundo;
        putnumber_i(1, 4, r->hora, 2);
        putnumber_i(1, 7, r->minuto, 2);
        putnumber_i(1, 10, r->segundo, 2);
    }
    if (r == A)
    {
        putmessage(1, 4, "alarme");
    }
}

void conta_cronometro(Rel *r)
{
    static char State = -1;

    if (Seleciona)
    {
        Seleciona = FALSE;
        State++;
        State %= 4;
    }

    if (r->segundoAnt != r->segundo)
    {
        r->segundoAnt = r->segundo;
        putnumber_i(1, 4, r->hora, 2);
        putnumber_i(1, 7, r->minuto, 2);
        putnumber_i(1, 10, r->segundo, 2);
    }
}

ISR(INT0_vect) // Pára o relógio  e Configura o Alarme
{
    Estado_Rel = ALARME_REL;
    Seleciona = TRUE;
}

ISR(INT1_vect) // Pára o relógio e prepara  o Cronometro
{
    Estado_Rel = CRONOMETRO;
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
