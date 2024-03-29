/*
	< DanaSah is a winboard/xboard and uci chess engine. >
    Copyright (C) <2018>  <Pedro Castro Elgarresta>
	E-mail <pecastro@msn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
Funciones para la busqueda de la mejor variante principal
*/

/*Includes*/
#include <stdio.h>									/* prinft, scanf, sprinft, sscanf, fgets, fflush... */
#include <string.h>     							/* menset, strcmp, strcat... */
#include <stdlib.h>     							/* abs */
#include <math.h>									/* pow */
#include "definiciones.h"
#include "variables.h"
#include "funciones.h"
#if defined (UNDER_CE) || defined (_WIN32)
	#include <windows.h>        					/* Sleep */
#else
	#include <unistd.h>
	#define Sleep(x) usleep((x)*1000)
#endif

/* ponemos en primer lugar el movimiento con mayor valor para la ordenacion */
void OrdenaMovimiento(int de, int cuenta, movimiento *lista)
{
	int i, j, Max;
	movimiento Tmp;

	/*cerca del maximo*/
	Max = lista[de].valor;
	j = de;
	for (i = (de+1); i < cuenta; i++) {
		if (lista[i].valor > Max) {
			Max = lista[i].valor;
			j = i;
		}
	}
	if (Max != lista[de].valor) {
		Tmp = lista[de];
		lista[de] = lista[j];
		lista[j] = Tmp;
	}
}

void OrdenaRootMayorMenor(void) {
         int i, j;
         movimiento aux;

         for (i = 0; i < root_contar -1 ; i++) {
                 for (j = i + 1; j < root_contar ; j++) {
                         if (root_lista[i].valor < root_lista[j].valor) {
                                aux = root_lista[i];
                                root_lista[i] = root_lista[j];
                                root_lista[j] = aux;
                         }
                }
         }
}

void InsertaMejormovimiento_root(void) {

    int i, j;

    /* calculamos la posici�n del movimiento que vamos a poner al principio de la lista */
	for (i = 0; i < root_contar;) {
        if (root_lista[i].de == pv[0][0].de && root_lista[i].a == pv[0][0].a && root_lista[i].tipo == pv[0][0].tipo)
            break;
        i++;
	}

    /* Desplazamos la lista de movimientos root para dejar espacio al nuevo mejor movimiento */
	for (j = i; j > 0; --j) {
        root_lista[j] = root_lista[j-1];
	}

    /* Mejor movimiento seg�n la ultima iteracion */
    root_lista[0] = pv[0][0];
}

/* actualizamos la variante principal con un movimiento */
void Actualiza_pv(movimiento pvmov)
{
	int j;

	pv[ply][ply] = pvmov;
	for (j = ply + 1; j < pv_longuitud[ply + 1]; ++j)
		pv[ply][j] = pv[ply + 1][j];
	pv_longuitud[ply] = pv_longuitud[ply + 1];
}

/* Esta funcion nos va a devolver el mejor movimiento y el posible a ponderar, desde ella se llamara a la funcion alpha-beta o equivalente */
void MotorPiensa(movimiento *mejormovimiento, movimiento *pondermovimiento)
{
	int	i, alpha, beta, contar_legales;
	char s[6];
	char inputline[80];
	int de, a, tipo;
	int contar;
    movimiento	lista[256];
	int tiempo_usado;
	int l;
	int njugadas_libro = 60;
	#if defined (UNDER_ELO)
	int mov_aleatorio;
	#endif

	/* iniciamos el mejor movimiento y el de ponderar con ninguno */
	*mejormovimiento = no_move;
	*pondermovimiento = no_move;

	/* calculamos el tiempo inicial de partida gracias a la hora actual del sistema */
	tiempo_inicio = get_ms();

	/* si estamos en el estado de buscando debemos comprobar el libro de aperturas */
	if (estado == BUSCANDO) {

		#if defined (UNDER_ELO)
		/* si estamos en la version limitada de fuerza */
		if (easylevel == 1) {										/* el motor debe hacer un movimiento aleatorio sin buscar */
			contar = Generar(lista);
			mov_aleatorio = rand() % contar;
			if (protocolo == UCI)
				printf("info string Random move\n");
			else
				printf ("0 0 0 0 [Random move]\n");
			*mejormovimiento = lista[mov_aleatorio];
			return;
		}
		/* si jugamos con elo o con un nivel reducido de fuerza limitamos tambien el libro de aperturas */
		if ((strcmp(limitstrength, "true") == 0) || easylevel > 1)	njugadas_libro =  elo / 100;
		#endif

		/* ahora vamos a comprobar si hay un movimiento de libro, si el libro esta cargado, se han juegado menos de 30
		movimientos y no se han jugado mas de 4 movimientos fuera del libro*/
		if ((strcmp (variante,"normal") == 0) && libro_cargado && fuera_del_libro < 3 && njugadas < njugadas_libro) {
			book_move(s);

			/* calculamos las coordenadas del movimiento, de, a y tipo y comparamos con una lista que generamos de todos
			los posibles movimientos, asi sabemos que el movimiento que viene del libro es legal */
			if (strcmp(s,"") != 0) {
				de = s[0] - 'a';
				de += 8 * (8 - (s[1] - '0'));
				a = s[2] - 'a';
				a += 8 * (8 - (s[3] - '0'));
				tipo = NORMAL;

				if (Comprobar_Tipo_Movimiento(de, a, s) != NINGUNO)
					tipo = Comprobar_Tipo_Movimiento(de, a, s);

				/* generamos una lista con todos los posibles movimientos que se podr�an jugar */
				contar = Generar(lista);
				/* recorre todos los movimientos para ver si es legal */
				for (i = 0; i < contar; i++) {
					if (lista[i].de == de && lista[i].a == a && lista[i].tipo == tipo) {
						if (Hacer_Movimiento(lista[i])) {
							Deshacer();			            		/* deshacemos el movimiento ya que lo enviamos luego a la GUI */
							if (protocolo == UCI)
								printf("info string Book move\n");	/* indicamos movimiento de libro para GUI con protocolo uci */
							else
								printf ("0 0 0 0 Book move\n");		/* indicamos movimiento de libro para GUI con protocolo xboard */
#if defined (UNDER_CE)
							fflush(stdout);							/* necesario en el caso de entorno PocketPc */
#endif
							*mejormovimiento = lista[i];			/* movimiento que retornamos y se enviar� al GUI */
							return;
						}
						else {
							Deshacer();	                    		/* si el movimiento del libro no es legal lo deshacemos */
							continue;
						}
					}
				}
			}
		}
	}
#if defined (UNDER_CE)
	/* en el caso del PocketPc indicamos que estamos ponderando y luego no enviaremos mas informacion ya que la GUI es muy lenta
	a la hora de mostrar dicha informacion y afecta a la hora de salir de este modo */
	else if (estado == PONDERANDO) {
		printf ("0 0 0 0 pondering...\n");
		fflush(stdout);
	}
#endif

	aborta = FALSO;													/* no podemos abortar la busqueda en el primer ply */
	para_busqueda = FALSO;                              			/* variable para saber cuando debemos abandonar la busqueda */

	ply = 0;					                        			/* comenzamos en el ply 0 (otros empiezan en el 1) */
	nodos = 0;					                        			/* lleva la cuenta del numero de nodos */

	material = piezasblanco+piezasnegro+peonesblanco+peonesnegro;	/* calcula el material todal de ambos lados */
	actualizar_info = FALSO;										/* no actualizamos informacion para la GUI hasta despues de 1 segundo (uci) */
	fail_low = 0; fail_high = 0; fail = NO_FAIL;					/* lleva cuantas veces nos hemos salido de la ventana en la busqueda */
	egtb_hits = 0;
	egbb_hits = 0;

	hacer_lazy_eval = 1;											/* realizamos lazy eval en la evaluacion */
	evalua_suma = 0;
    evalua_cuenta = 1;

	/* el motor tiene que comprobar si tiene que salir de la busqueda, una comprobacion por cada nodo es algo costoso, asi que trataremos
	de hacer esta comprobacion cada mas nodos si es posible, cknodos aqui esta puesto en hexadecimal */
#if defined (UNDER_ELO)
	/* en la version limitada de elo la comprobacion es mas amenudo para evitar perdidas por tiempo */
	if (material<FINAL)
		cknodos =  0x1;
	else
		cknodos =  0x10;
#elif defined (UNDER_CE)
	/* en la version de PocketPc parece que estos numeros son adecuados */
	if (material<FINAL)
		cknodos =  0x33;
	else
		cknodos =  0xCCC;
#else
	/* para el resto de versiones parece que estos numeros son ok para no perder en el tiempo, incluso para android */
	if (material<FINAL)
		cknodos =  0x400;
	else
		cknodos =  0x4000;
#endif

#if defined (UNDER_ELO)
	/* solo en la version limitada en fuerza si jugamos con un determinado elo o con nivel facil tenemos que limitar el numero de nodos
	por segundo que hace el motor (nps), la formula mas o menos tiene en cuenta que cada vez que duplicamos el numero de nodos vamos a
	ganar un ply de profundidad y un elo entorno a los 100 puntos */
	if (elo >= 2100) nps_elo = pow(2,((double)elo)/100.0) / 100;
	else if (elo >= 1400) nps_elo = (pow(2,((double)elo)/100.0) / 100) * ((2200 - elo) / 100);
	else nps_elo = 1000;
#endif

	/* borramos la pv, historia, historia maxima y los movimientos killer y matekiller */
	memset(pv, 0, sizeof(pv));
    /*memset(Historia, maxhistoria, sizeof(Historia));*/
    for (de = 0; de < 64; de++) {
        for (a = 0; a < 64; a++) {
            Historia[de][a] /= 8;
        }
    }
	memset(matekiller, 0, sizeof(matekiller));
	memset(killer1, 0, sizeof(killer1));
	memset(killer2, 0, sizeof(killer2));
	maxhistoria = 0;

	/* guardamos el material que hay en este momento para utilizarlo luego en la evaluacion */
	old_piezasblanco = piezasblanco;
	old_piezasnegro = piezasnegro;
	old_peonesblanco = peonesblanco;
	old_peonesnegro = peonesnegro;

	/* definimos ventanas para hacer busqueda, en lugar de buscar desde -MATE hasta MATE que ser�an todas las posibles evaluaciones,
	buscamos desde -WINDOW a +WINDOW, asi la b�squeda es mas rapida, si la b�squeda sale de estos valores abriremos a una ventana mas
	amplia y finalmente hasta -MATE y MATE */
	#define WINDOW1	15
	#define WINDOW2	30
	#define WINDOW3	60
    #define WINDOW4	120
    #define WINDOW5 240

	/* Generamos la lista de movimientos legales */
	contar_legales = Generar_root_legales();
	//OrdenaRootMayorMenor();
	/*printf("El n�mero de movimientos legales es: %d\n", root_legales);*/

	root_jaque = EstaEnJaque(turno);

	/* vamos a llamar a la funcion RAIZ de una forma interactiva, primero buscamos completo la profundidad 1, luego la 2, etc, esto tiene
	2 ventajas, si el tiempo se termina al menos tenemos una busqueda completa en una profundidad y ademas cada nueva profundidad podemos
	resolverla mas rapido ya que se produce una mejor ordenaci�n de la variante principal */
	for (i = 1; i <= max_profundidad;) {

		li = i;	                				/* guardamos profundidad por si tenemos que utilizarla fuera de esta funcion */
		sigue_Pv = VERDADERO;					/* inicialmente hacemos que siga la variante principal */

		/* para las primeras profundidades hacemos una busqueda completa y sin consulta a las bitbases */
		if (i <= 6) {
			alpha = -MATE;
			beta = MATE;
		}

		/* llamamos a la funcion RAIZ, la profundidad que vamos a utilizar sera fracional, PLY = 8,	las extensiones fraccionales deberian
		permitir un mejor control */
		valor[i] = RAIZ(alpha, beta, i * PLY);

		/* sacamos el mejor movimiento y el movimiento a ponderar de la variante principal */
		l = pv_longuitud[0];
		if (l >= 1) {
			*mejormovimiento = pv[0][0];
			if (l >= 2) *pondermovimiento = pv[0][1];
			else *pondermovimiento = no_move;
		}

		/* si la busqueda ha terminado al superar el tiempo, salimos de la funcion */
		if (para_busqueda == VERDADERO) {
			break;
		}

		/* abrimos nuevas ventanas si se produce un valor fuera de la ventana durante la busqueda, parece funcionar bien y r�pido*/
		if (valor[i] <= alpha) {
			fail_low++;
			fail = LOW;
            if (i > 6) print_pv(i, valor[i]);
			if (fail_low == 1) {
				alpha = valor[i] - WINDOW2;
				continue;
			}
			if (fail_low == 2) {
				alpha = valor[i] - WINDOW3;
				continue;
			}
			if (fail_low == 3) {
				alpha = valor[i] - WINDOW4;
				continue;
			}
			if (fail_low == 4) {
				alpha = valor[i] - WINDOW5;
				continue;
			}
			if (fail_low == 5) {
				alpha = -MATE+200;
				continue;
			}
			alpha = -MATE;
			continue;
		}

		if (valor[i] >= beta) {
			fail_high++;
			fail = HIGH;
			if (i > 6) print_pv(i, valor[i]);
			if (fail_high == 1) {
				beta = valor[i] + WINDOW2;
				continue;
			}
			if (fail_high == 2) {
				beta = valor[i] + WINDOW3;
				continue;
			}
			if (fail_high == 3) {
				beta = valor[i] + WINDOW4;
				continue;
			}
			if (fail_high == 4) {
				beta = valor[i] + WINDOW5;
				continue;
			}
			if (fail_high == 5) {
				beta = MATE-200;
				continue;
			}
			beta = MATE;
			continue;
		}

        /* Visualiza la variante principal al final de cada iteraci�n */
        fail_low = 0; fail_high = 0; fail = NO_FAIL;
        print_pv(i, valor[i]);

		/* miramos si podemos reducir el tiempo de la busqueda */
		if (estado == BUSCANDO && i > 5) {
			/* si ya tenemos una profundidad minima podemos abortar la busqueda si es necesario */
			 aborta = VERDADERO;
			/* con un solo movimiento legal terminamos de buscar */
			if (contar_legales == 1) break;
			/* no comenzamos una nueva iteracion si ya hemos consumido al menos la mitad de tiempo */
			tiempo_usado = get_ms() - tiempo_inicio;
			if (tiempo_usado > no_nueva_iteracion) break;
		}

		/* Inserta el mejor movimiento de la ultima iteracion al principio de la lista */
		/*if (i > 6) {
		    if (pv[0][0].de != root_lista[0].de || pv[0][0].a != root_lista[0].a || pv[0][0].tipo != root_lista[0].tipo) {
                InsertaMejormovimiento_root();
		    }
		}*/

		/* configuramos la ventana para el siguiente ciclo e iniciamos el estado y cuenta de fail_low y fail_high */
		alpha = valor[i] - WINDOW1;
	    beta = valor[i] + WINDOW1;

	    /* enviamos informacion extra a la GUI en el protocolo uci si al menos ha pasado 1 segundo */
		tiempo_usado = get_ms() - tiempo_inicio;
		if (tiempo_usado > 1000) actualizar_info = VERDADERO;

		/* pasamos al siguiente nivel de profundidad */
	    i++;
	}
	/* si estamos poderando en el protocolo uci no podemos salir de este modo hasta que recibamos un comando stop o ponderhit */
	if (i > max_profundidad && estado == PONDERANDO && protocolo == UCI && para_busqueda == FALSO)
	{
		for (;;) {
			/* comprobamos si recibimos algo de la GUI */
			if (bioskey()) {
				/* obtenemos el comando */
				readline(inputline, sizeof(inputline));
				/* el comando quit haria terminar al motor */
				if (strcmp(inputline, "quit") == 0) {
					printf("Quitting during ponder, bye\n");
					exit(0);
				}
				/* el comando stop hace terminar el ponder y empezar un nueva busqueda */
				if (strcmp(inputline, "stop") == 0) {
					return;
				}
				/* el comando ponderhit haria que el motor siguiera pensando pero cambiado el estado de ponderando a buscando */
				if (strcmp(inputline, "ponderhit") == 0) {
					return;
				}
				/* no sera habitual recibir el comando isready */
				if (strcmp(inputline, "isready") == 0) {
					printf("readyok\n");
				}
			}
			Sleep(1);
		}
	}
	return;
}


/* la siguiente funcion, que solo se aplica en el root (raiz) o ply = 0, es la encargada de comenzar la busqueda, es un algoritmo alphabeta,
una variante que se llama variante principal, esta basada en un algoritmo de Bruce Moreland. La busqueda en el ply 0 se comporta un poco
diferente que el resto de profundidades por lo que es interesante tener una funcion en exclusiva dedicado a ello y ademas nos permitira
en un futuro tener una ordenacion tambien diferente */
int	RAIZ(int alpha, int beta, int depth)
{
	int i, nuMoves = 0, de, a;
	movimiento mejor_movimiento = no_move;
	movimiento lista[256];	/*218 es el m�ximo n�mero de combinaciones posibles para un movimiento*/

	int ext;
	int puntos;
	int mejor = - MATE;

	/*TTables*/
	char htipo;
	movimiento hmov;
	char hash_flag = hasfALPHA;
	int hacer_nulo;

	/* incrementamos el numero de nodos visitados */
	nodos++;

	/* comprobamos las tablas hash solo para ordenar */
	puntos = probe_hash(&htipo, depth+njugadas, beta, &hmov, &hacer_nulo);

	pv_longuitud[ply] = ply;

	/* Generamos la lista de movimientos legales */
	Generar_root_legales();
	//OrdenaRootMayorMenor();

	/* recupera todos los movimientos para el root */
	for (i = 0; i < root_contar; ++i)
		lista[i] = root_lista[i];

	/* ordenamos primero el movimiento que viene de la hash y luego el de la variante principal */
	Ordena_hmove_pv(root_contar, lista, hmov);

	/* recorremos todo el ciclo de movimientos */
    for (i = 0; i < root_contar; ++i) {
        //int cnodos = nodos;
		OrdenaMovimiento(i, root_contar, lista);   /* ordena primero el movimiento con mayor valor */
		if (!Hacer_Movimiento(lista[i]))
			continue;
		nuMoves++;	    									/* numero de movimientos legales recorridos */

		/* mostramos valoracion extra por cada movimiento en el protocolo uci */
		if (protocolo == UCI && actualizar_info) {
			printf("info depth %d", li);
			printf(" currmove");
			printf(" %c%d%c%d",'a' + COLUMNA(lista[i].de),	8 - FILA(lista[i].de), 'a' + COLUMNA(lista[i].a), 8 - FILA(lista[i].a));
			printf(" currmovenumber %d\n", nuMoves);
		}

		/* si el movimiento lo preveemos complicado extenderemos la profundidad con hasta un PLY */
		ext = 0;
		material = piezasblanco+piezasnegro+peonesblanco+peonesnegro;
		/* damos jaque */
		if (EstaEnJaque(turno) && see2(turno, lista[i].a, lista[i].de) >= 0) ext += PLY;
		/* peon en septima */
		else if (material < MEDIOJUEGO && pieza[jugadas[njugadas-1].m.a] == PEON
			&& (FILA(jugadas[njugadas-1].m.a) == 1 || FILA(jugadas[njugadas-1].m.a) == 6))
			ext += PLY;
        /*recaptura*/
		else if (jugadas[njugadas-1].captura != VACIO && jugadas[njugadas-2].captura != VACIO
			&& jugadas[njugadas-1].m.a == jugadas[njugadas-2].m.a
			&& abs(valor_pieza[jugadas[njugadas-1].captura] - valor_pieza[jugadas[njugadas-2].captura] <= VALOR_PEON/2))
			ext += PLY;

		/* para el primer movimiento hacemos una busqueda completa */
		if (nuMoves == 1) {
			puntos = -PVS(-beta, -alpha, depth+ext-PLY, VERDADERO);
		}
		else {
			/*probamos a hacer LMR si no hay extensiones, si hay un numero minimo de movimientos legales, si tenemos una determinada
			profundidad, si no es el movimiento de la tabla	hash, variante principal, matekiller, capturas de ganancia e iguales,
			promociones y killers y que su historia no sea elevada, si todo esto se cumple probamos a reducir la profundidad */
			if (nuMoves >= 4 && depth >= 2*PLY && !ext && !root_jaque && !sigue_Pv && alpha > (-MATE/2) && lista[i].valor < 1500000) {
				puntos = -PVS( -alpha-1, -alpha, depth-2*PLY, VERDADERO);
			}
			else
				puntos = alpha+1;								/* nos aseguramos que una busqueda completa es hecha */
			if (puntos > alpha) {
				puntos = -PVS(-alpha-1, -alpha, depth+ext-PLY, VERDADERO);
				if (para_busqueda == FALSO && puntos > alpha && puntos < beta)
					puntos = -PVS(-beta, -alpha, depth+ext-PLY, VERDADERO);
			}
		}

		//lista[i].valor = nodos - cnodos;
		//root_lista[i] = lista[i];

		/* Deshacemos el movimiento */
		Deshacer();

		/* Salimos de la busqueda si se ha terminado el tiempo */
		if (para_busqueda == VERDADERO)
			return 0;

		if (puntos > mejor) {
			mejor = puntos;
			mejor_movimiento = lista[i];

			if (puntos > alpha) {
				alpha = puntos;
				hash_flag = hasfEXACT;
				Actualiza_pv(mejor_movimiento);	    			/* actualizamos la variante principal */
				/* actualizamos la historia si el movimiento mejora lo anterior */
				if (!root_jaque && !ext && pieza[mejor_movimiento.a] == VACIO && mejor_movimiento.tipo < PROMOCION_A_DAMA) {
					Historia[mejor_movimiento.de][mejor_movimiento.a] += (depth/PLY) * (depth/PLY);
					if (Historia[mejor_movimiento.de][mejor_movimiento.a] > maxhistoria)
						maxhistoria = Historia[mejor_movimiento.de][mejor_movimiento.a];
					if (maxhistoria > 2000000) {
						maxhistoria /=2;
                        /*memset(Historia, maxhistoria, sizeof(Historia));*/
                        for (de = 0; de < 64; de++)
                            for (a = 0; a < 64; a++)
                                Historia[de][a] /= 2;
					}
				}
				if (puntos >= beta) {
					/* Los movimientos matekiller y killer son movimientos buenos, aprovechamos para guardar informacion de ellos */
					if (puntos > (MATE-64))
						matekiller[ply] = mejor_movimiento;		/*un movimiento matekiller, es un movimiento killer (asesino) que es un mate*/
					else if (!root_jaque && pieza[mejor_movimiento.a] == VACIO && mejor_movimiento.tipo < PROMOCION_A_DAMA) {
						/* guardamos dos movimientos killer que no sean movimientos de captura o promocion */
						if ((mejor_movimiento.a != killer1[ply].a) || (mejor_movimiento.de != killer1[ply].de)) {
							killer2[ply] = killer1[ply];
							killer1[ply] = mejor_movimiento;
						}
					}
					/* Almacenamos en las tablas hash un tipo lower */
					store_hash(hasfBETA, depth+njugadas, mejor, mejor_movimiento);
					return mejor;
				}
				//Actualiza_pv(mejor_movimiento);	    			/* actualizamos la variante principal */
#if defined (UNDER_CE)
				/* en la version PocketPC solo imprimimos la variante princial cuando alcanzamos la profunidad 6 y solo durante la busqueda */
				if (estado == BUSCANDO && li > 5) print_pv(li, puntos);
#else
				/* imprimimos la variante principal despues de alcanzar la profunidad 3, en las 2 primeras profundidades puede haber muchos
				cambios */
				if (li > 6) print_pv(li, puntos);
#endif
			}
			/* si el movimiento no mejora alpha lo tenemos en cuenta para la historia */
			else {
				if ((pieza[lista[i].a] == VACIO) && lista[i].tipo < PROMOCION_A_DAMA) {
					Historia[lista[i].de][lista[i].a] -= (depth/PLY);
				}
			}
		}
	}

	/* si no hay movimiento legales */
	if (nuMoves == 0) {
		if (EstaEnJaque(turno)) mejor = -MATE + ply; 			/* mate */
		else mejor = 0; 										/* ahogado */
	}

	/* Almacenamos en las tablas hash un tipo exact o upper segun corresponda */
	store_hash(hash_flag, depth+njugadas, mejor, mejor_movimiento);
	return mejor;
}


/* la siguiente funcion es la encargada de la busqueda para cualquier ply excepto el cero, es un algoritmo alphabeta, una variante
que se llama variante principal, esta basada en un algoritmo de Bruce Moreland */
int	PVS(int alpha, int beta, int depth, int hacer_nulo)
{
	int i, jaque, contar = 0, nuMoves = 0, ev, total_piezas, amenaza_mate = 0, R = 2, vp = 0, de, a;
	int hacer_FP = 0, fmax = 0, hacer_LMP = 0, hacer_SP = 0;
	movimiento mejor_movimiento = no_move;
	movimiento lista[256];   /*218 es el m�ximo n�mero de combinaciones posibles para un movimiento*/

    int nodo_pv =  ((beta - alpha) > 1);
	int ext;
	int puntos = alpha;
	int mejor = -MATE;

	/*TTables*/
	char htipo;
	movimiento hmov;
	char hash_flag = hasfALPHA;

	/* si estamos en profundidad cero o en nuestro caso menor que un PLY llamamos al buscador de capturas y promociones */
	/* lo mismo si hemos ido muy profundo */
	if ((depth < PLY) || ply >= MAX_PROFUNDIDAD-1) {
		return Extiende_capturas(alpha, beta, 0);
	}

	nodos++;													/* incrementamos el numero de nodos */
	if ((nodos % cknodos) == 0) 	        					/* comprueba cada un cierto numero de nodos que tenemos tiempo */
    {
        checkup();
        /*printf("Lazy_Eval = %d Nodos = %d\n", lazy_eval, nodos);*/
    }
	if (para_busqueda == VERDADERO)								/* salimos si se acabo el tiempo */
		return 0;

	/* mate-distance pruning permite buscar mates mas corto mas rapido */
	alpha = MAX(alpha, -MATE + ply);
	beta = MIN(beta, MATE - ply);
	if (alpha >= beta)
		return alpha;

	/* si la posici�n se repite 1 vez se valora como tablas, aunque no las reclama */
	if (repeticion () > 1)
		return 0;

#if defined (UNDER_ELO)
	/* en la version limitada en fueza si jugamos con elo o nivel facil tenemos que regular el numero de nodos por segundo */
	regular_nps();
#endif

	/* comprobamos las tablas hash y vemos si podemos cortar la busqueda */
	puntos = probe_hash(&htipo, depth+njugadas, beta, &hmov, &hacer_nulo);
	if (puntos != valUNKNOWN)  {
		switch (htipo) {
			case hasfEXACT:									/* tipo exact */
				/* paramos la busqueda excepto si estamos en un nodo-pv ya que nos acortaria la variante principal */
				if (!nodo_pv) return puntos;
				break;
			case hasfALPHA:									/* tipo upper */
				if (puntos <= alpha) return alpha;
				break;
			case hasfBETA:									/* tipo lower */
				if (puntos >= beta) return beta;
				break;
			case hasfNULL:
				break;
            default:
                break;
		}
	}

	/* comprobamos si estan cargadas las bitbases de Scorpio y las tablebases de Gaviota */
	if (egtb_is_loaded || egbb_is_loaded) {
		total_piezas = peonesblancos + caballosblancos + alfilesblancos + torresblancas
						+ damasblancas + peonesnegros + caballosnegros + alfilesnegros + torresnegras + damasnegras + 2;
		/* Primero probamos en conjunto bitbases con tablebases si las bitbases son mas completas */
		if (egbb_is_loaded && egtb_is_loaded && total_piezas <= egbb_men) {
            int score;
			if (egbb_men > egtb_men && total_piezas > egtb_men) {
                int plimit = (li)*2/3;
                int dlimit = 5*PLY;
                if ((depth >= dlimit || total_piezas <= 4) && (ply >= plimit || regla50 == 0)) {
                    if (probe_bitbases(&score) == 1) {
                        if (score>MATE-200) score = MATE-ply;
                        else if (score <-MATE+200) score = -MATE+ply;
                        if (score > GANADA) {
                            if (score > MATE-64) score -= ply;
                            else if (score < MATE-64) score -= 40*ply;
                        }
                        else if (score < -GANADA) {
                            if (score < -MATE+64) score += ply;
                            else if (score > -MATE+64) score += 40*ply;
                        }
                        return score;
                    }
                }
			}
			if ((ply == 1) && probe_tablebases(&score, DTM_hard) == 1) {
				if (score > 0)	score = score - ply;
				else if (score < 0)	score = score + ply;
				return score;
			}
			if ((depth >= 4*PLY || (nodo_pv && depth >= 1*PLY)) && probe_tablebases(&score, (alpha<=-MATE+200 || beta>=MATE-200)? DTM_hard:WDL_hard) == 1) {
				if (score > 0)	score = score - ply;
				else if (score < 0)	score = score + ply;
				return score;
			}
			if ((nodo_pv || depth >= 1*PLY) && probe_tablebases(&score, (alpha<=-MATE+200 || beta>=MATE-200)? DTM_soft:WDL_soft) == 1) {
				if (score > 0)	score = score - ply;
				else if (score < 0)	score = score + ply;
				return score;
			}
		}
		/* Probamos tablebases si no estan cargadas bitbases */
		if (egtb_is_loaded && !egbb_is_loaded && total_piezas <= egtb_men) {
			int score;
			if ((ply == 1) && probe_tablebases(&score, DTM_hard) == 1) {
				if (score > 0)	score = score - ply;
				else if (score < 0)	score = score + ply;
				return score;
			}
			if ((depth >= 4*PLY || (nodo_pv && depth >= 1*PLY)) && probe_tablebases(&score, (alpha<=-MATE+200 || beta>=MATE-200)? DTM_hard:WDL_hard) == 1) {
				if (score > 0)	score = score - ply;
				else if (score < 0)	score = score + ply;
				return score;
			}
			if ((nodo_pv || depth >= 1*PLY) && probe_tablebases(&score, (alpha<=-MATE+200 || beta>=MATE-200)? DTM_soft:WDL_soft) == 1) {
				if (score > 0)	score = score - ply;
				else if (score < 0)	score = score + ply;
				return score;
			}
		}
		/* Probamos bitbases si las tablebases no estan cargadas */
		if (egbb_is_loaded && !egtb_is_loaded && total_piezas <= egbb_men) {
			int score;
			int plimit = (li)*2/3;
			int dlimit = 5*PLY;
			if ((depth >= dlimit || total_piezas <= 4) && (ply >= plimit || regla50 == 0)) {
				if (probe_bitbases(&score) == 1) {
					if (score>MATE-200) score = MATE-ply;
					else if (score <-MATE+200) score = -MATE+ply;
					if (score > GANADA) {
						if (score > MATE-64) score -= ply;
						else if (score < MATE-64) score -= 40*ply;
					}
					else if (score < -GANADA) {
						if (score < -MATE+64) score += ply;
						else if (score > -MATE+64) score += 40*ply;
					}
					return score;
				}
			}
		}
	}

	jaque = EstaEnJaque(turno);								/* comprobamos si estamos en jaque */

	/* la mayoria de tecnicas tratando de cortar o reducir el arbol necesitan que en el rey de quien mueve no este en jaque, que
	no estemos en un nodo_pv o que beta no este cerca del MATE*/
	if (!jaque && !nodo_pv && alpha > -MATE+64 && beta < MATE-64) {

		ev = Evaluar(-MATE, MATE);							/* obtenemos la evaluacion para el nodo */

        /* razoring */
        if (depth <= 3*PLY && hmov.a == 0 && hmov.de == 0) {
            int margen[4] = {0, 160, 240, 320};
            if (ev + margen[depth/PLY] <= alpha) {
                if (depth <= PLY) {
                    puntos = Extiende_capturas(alpha, beta, 0);
                    return MAX(puntos, ev + margen[depth/PLY]);
                }
                int ralpha = alpha-margen[depth/PLY];
                puntos = Extiende_capturas(ralpha, ralpha+1, 0);
                if (puntos <= ralpha)
                    return puntos;
            }
        }

		if (hacer_nulo && ((turno == BLANCO && piezasblanco > VALOR_ALFIL) || (turno == NEGRO && piezasnegro > VALOR_ALFIL))){

            /* static null move o reverse futility pruning */
            if (depth < 7*PLY) {
                int margen[7] = {0, 150, 300, 450, 600, 750, 900 };
                if (ev - margen[depth/PLY] >= beta) {
                    if (depth > 3*PLY)
                        depth -= PLY;
                    else
                        return (ev);
                }
            }

            /* movimiento nulo */
			if (depth > PLY && ev >= beta) {

				/* utilizamos un movimiento nulo agresivo, algo caracteristico de danasah desde practicamente la primera version */
				material = piezasblanco+piezasnegro+peonesblanco+peonesnegro;
				depthnull = depth - (1+R+(material>MEDIOJUEGO)+(depth>6*PLY)+(depth>11*PLY)+(depth>16*PLY))*PLY;

				/* hacemos el movimiento nulo, actualizamos la llave hash */
				jugadas[njugadas].m = no_move;
				jugadas[njugadas].captura = VACIO;
				jugadas[njugadas].enroque = enroque;
				jugadas[njugadas].alpaso = alpaso;
				jugadas[njugadas].regla50 = regla50;
				jugadas[njugadas].hash = hash;
				if (alpaso != -1)
					hash ^= hash_alpaso[alpaso];
				alpaso = -1;
				regla50++;
				ply++;
				njugadas++;
				hash ^= hash_turno[turno];
				turno = 1 - turno;
				hash ^= hash_turno[turno];

				if (depthnull < PLY)
                    puntos = -Extiende_capturas(-beta, -beta+1, 0);
                else
                    puntos = -PVS(-beta, -beta+1, depthnull, FALSO);

				/* deshacemos el movmiento nulo */
				turno = 1 - turno;
				njugadas--;
				ply--;
				enroque = jugadas[njugadas].enroque;
				alpaso = jugadas[njugadas].alpaso;
				regla50 = jugadas[njugadas].regla50;
				hash = jugadas[njugadas].hash;

				/* salimos si se acabo el tiempo */
				if (para_busqueda == VERDADERO)
					return 0;

				/* si la busqueda reducida todavia tiene un valor lo suficientemente bueno */
				if (puntos >= beta) {
                    if (puntos >= MATE-64) puntos = beta;   /* no devuelve un mate que no se produce */
                    if (depth < 12*PLY || material>FINAL) {
                        store_hash(hasfBETA, depth+njugadas, puntos, mejor_movimiento);
                        return puntos;
                    }
                    depth -= 5 * PLY;

                    /*if (depthnull < PLY)
                        vp = Extiende_capturas(beta-1, beta, 0);
                    else
                        vp = PVS(beta-1, beta, depthnull, 0);

                    if (vp >= beta) {
                        store_hash(hasfBETA, depth+njugadas, puntos, mejor_movimiento);
                        return puntos;
                    }*/
                }
				/* comprueba si tenemos una amenaza */
				else if (puntos <= -(MATE/2) && beta > -(MATE/2)) amenaza_mate = 1;
			}
		}

		/* pruning */
		if (depth < 7*PLY) {
            int margen[7] = {0, 100, 200, 300, 400, 500, 600 };
			hacer_LMP = 1;
			hacer_SP = 1;
			fmax = ev + margen[depth/PLY];
			if (fmax <= alpha)
				hacer_FP = 1;
		}
	}

	/* Internal Iterative Deepening si no tenemos un movimiento de las tablas hash y hemos alcanzado una determinada profundidad,
	ayuda a tener un movimiento sobre el que empezar la busqueda */
	if (hmov.a == 0 && hmov.de == 0 && (depth >= 11*PLY || (depth >= 6*PLY && nodo_pv))) {
		puntos = PVS(alpha, beta, nodo_pv ? depth-2*PLY : depth/2, VERDADERO);
		probe_hash(&htipo, depth+njugadas, beta, &hmov, &hacer_nulo);
	}

	pv_longuitud[ply] = ply;
	contar = Generar(lista);	    						/* genera todos los movimientos para la posici�n actual */

	/* ordenamos primero el movimiento que viene de la hash y luego el de la variante principal */
	Ordena_hmove_pv(contar, lista, hmov);

	/* recorremos todo el ciclo de movimientos */
    for (i = 0; i < contar; ++i) {
        OrdenaMovimiento(i, contar, lista);

        /* futility prunning de no capturas */
        if (hacer_FP && nuMoves > 1) {
            if (lista[i].valor < 900000)
                continue;
        }

		if (!Hacer_Movimiento(lista[i]))
			continue;
		nuMoves++;	    									/* numero de movimientos legales */

		/* si el movimiento lo preveemos complicado extenderemos la profundidad con hasta un PLY, mas si estamos en un nodo_pv */
		ext = 0;
		material = piezasblanco+piezasnegro+peonesblanco+peonesnegro;
		if (nodo_pv) {
			/* damos jaque */
			if (EstaEnJaque(turno) && see2(turno, lista[i].a, lista[i].de) >= 0) ext += PLY;
			/* tenemos una amenaza */
			else if (amenaza_mate) ext += PLY;
			/* peon en septima */
			else if (material < MEDIOJUEGO && pieza[jugadas[njugadas-1].m.a] == PEON
				&& (FILA(jugadas[njugadas-1].m.a) == 1 || FILA(jugadas[njugadas-1].m.a) == 6))
				ext += PLY;
			/*recaptura*/
			else if (jugadas[njugadas-1].captura != VACIO && jugadas[njugadas-2].captura != VACIO
				&& jugadas[njugadas-1].m.a == jugadas[njugadas-2].m.a
				&& abs(valor_pieza[jugadas[njugadas-1].captura] - valor_pieza[jugadas[njugadas-2].captura] <= VALOR_PEON/2))
				ext += PLY;
		}
		else {
			if (EstaEnJaque(turno) && see2(turno, lista[i].a, lista[i].de) >= 0) ext += PLY/2;
			else if (amenaza_mate) ext += PLY/2;
			else if (material < MEDIOJUEGO && pieza[jugadas[njugadas-1].m.a] == PEON
				&& (FILA(jugadas[njugadas-1].m.a) == 1 || FILA(jugadas[njugadas-1].m.a) == 6))
				ext += PLY/2;
		}

		/* para el primer movimiento hacemos una busqueda completa */
		if (nuMoves == 1) {
			puntos = -PVS(-beta, -alpha, depth+ext-PLY, VERDADERO);
		}
		else {
            if (!ext) {
                /* futiliy pruning de capturas */
                if (hacer_FP) {
                    if (jugadas[njugadas-1].captura != VACIO && fmax + valor_pieza[jugadas[njugadas-1].captura] <= alpha){
                        Deshacer();
                        continue;
                    }
                }

                /* LMP */
                if (hacer_LMP) {
                    if (lista[i].valor < 900000 && nuMoves > 4 + (depth/PLY) * (depth/PLY)) {
                        Deshacer();
                        return mejor;
                    }
                }

                /* SEE prunning */
                if (hacer_SP) {
                    if (jugadas[njugadas-1].captura != VACIO) {
                        if (see2(turno, lista[i].a, lista[i].de) < (-120*(depth/PLY))) {
                            Deshacer();
                            continue;
                        }
                    }
                    /*else {
                        if (see2(turno, lista[i].a, lista[i].de) < (-35*(depth/PLY)*(depth/PLY))) {
                            Deshacer();
                            continue;
                        }
                    }*/
                }
            }

			/*probamos a hacer LMR si no hay extensiones, si hay un numero minimo de movimientos legales, si tenemos una determinada
			profundidad, si no es el movimiento de la tabla	hash, variante principal, matekiller, capturas de ganancia e iguales,
			promociones y killers y que su historia no sea elevada, si todo esto se cumple probamos a reducir la profundidad */
			if (nuMoves >= 3 && depth >= 2*PLY && !jaque && alpha > (-MATE/2) && !ext && lista[i].valor < 1500000) {
                int r;
                if (nuMoves > 36 && depth >= 8*PLY && lista[i].valor < 22500)
					r = 8;
                else if (nuMoves > 30 && depth >= 7*PLY && lista[i].valor < 45000)
					r = 7;
                else if (nuMoves > 24 && depth >= 6*PLY && lista[i].valor < 90000)
					r = 6;
                else if (nuMoves > 18 && depth >= 5*PLY && lista[i].valor < 180000)
					r = 5;
				else if (nuMoves > 12 && depth >= 4*PLY && lista[i].valor < 360000)
					r = 4;
				else if (nuMoves > 6 && depth >= 3*PLY && lista[i].valor < 720000)
					r = 3;
                else
                    r = 2;
                if (nodo_pv) r = r*2/3;
                if (r < 2) r = 2;
                puntos = -PVS( -alpha-1, -alpha, depth-r*PLY, VERDADERO);
			}
			else
				puntos = alpha+1;       					/* nos aseguramos que una profundidad completa es hecha */
			if (puntos > alpha) {
				puntos = -PVS(-alpha-1, -alpha, depth+ext-PLY, VERDADERO);
				if (para_busqueda == FALSO && puntos > alpha && puntos < beta)
					puntos = -PVS(-beta, -alpha, depth+ext-PLY, VERDADERO);
			}
		}

		/* Deshacemos el movimiento */
		Deshacer();

		/* Salimos si se acabo el tiempo */
		if (para_busqueda == VERDADERO)
			return 0;

		/* si el resultado es mejor que cualquier previo */
		if (puntos > mejor) {
			mejor = puntos;									/* tenemos un mejor resultado a devolver */
			mejor_movimiento = lista[i];					/* tenemos un nuevo mejor movimiento para la variante principal */
			/* comprobamos si el resultado es mejor que alpha */
			if (puntos > alpha) {
				alpha = puntos;								/* actualizamos alpha */
				hash_flag = hasfEXACT;						/* tenemos como tipo un exact */
				Actualiza_pv(mejor_movimiento);	    		/* actualizamos la variante principal */
				/* actualizamos la historia y maxima historia*/
				if (!jaque && !ext && pieza[mejor_movimiento.a] == VACIO && mejor_movimiento.tipo < PROMOCION_A_DAMA) {
					Historia[mejor_movimiento.de][mejor_movimiento.a] += (depth/PLY) * (depth/PLY);
					if (Historia[mejor_movimiento.de][mejor_movimiento.a] > maxhistoria)
						maxhistoria = Historia[mejor_movimiento.de][mejor_movimiento.a];
					/* si pasamos de un cierto valor reducimos a la mitad el valor de la historia y maxima historia */
					if (maxhistoria > 2000000) {
						maxhistoria /=2;
                        /*memset(Historia, maxhistoria, sizeof(Historia));*/
                        for (de = 0; de < 64; de++)
                            for (a = 0; a < 64; a++)
                                Historia[de][a] /= 2;
					}
				}
				/* comprobamos si el resultado es mejor o igual a beta */
				if (puntos >= beta) {
					/* Los movimientos matekiller y killer son movimientos buenos, aprovechamos para guardar informacion de ellos */
					if (puntos > (MATE-64))
						matekiller[ply] = mejor_movimiento;		/*un movimiento matekiller, es un movimiento killer que es un mate */
					else if (!jaque && pieza[mejor_movimiento.a] == VACIO && mejor_movimiento.tipo < PROMOCION_A_DAMA) {
						/* guardamos dos movimientos killer que no sean movimientos de captura o promocion */
						if ((mejor_movimiento.a != killer1[ply].a) || (mejor_movimiento.de != killer1[ply].de)) {
							killer2[ply] = killer1[ply];
							killer1[ply] = mejor_movimiento;
						}
					}
					/* Almacenamos un tipo lower en las tablas hash */
                    store_hash(hasfBETA, depth+njugadas, mejor, mejor_movimiento);
					return mejor;
				}
				//Actualiza_pv(mejor_movimiento);	    		/* actualizamos la variante principal */
			}
			/* penalizamos algo la historia del movimiento */
			else {
				if ((pieza[lista[i].a] == VACIO) && lista[i].tipo < PROMOCION_A_DAMA) {
					Historia[lista[i].de][lista[i].a] -= (depth/PLY);
				}
			}
		}
	}

	/* si no tenemos movimientos legales */
	if (nuMoves == 0) {
		if (jaque) mejor = -MATE + ply; 					/* mate */
		else mejor = 0; 									/* ahogado */
	}
	/* si hemos superado la regla de los 50 movimientos tendremos que evaluar la posicion como tablas */
	else if (regla50 >= 100)
		mejor = 0;

	/* Almacenamos en las tablas hash un tipo exact o upper segun corresponda */
	store_hash(hash_flag, depth+njugadas, mejor, mejor_movimiento);
	return mejor;
}


/* extiende capturas es llamado en otros motores como quiesce, es similar a alphabeta salvo en que solo busca capturas y promociones,
no lleva profundidad, esta funcion sirve para evitar el efecto horizonte */
int Extiende_capturas(int alpha, int beta, int depth)
{
	int i, contar2, delta = 0, total_piezas, jaque, legal = 0;
	movimiento lista2[256], hmov = no_move;

	int puntos = alpha, mejor = -MATE, evalua_estatica = 0;
	int nodo_pv =  ((beta - alpha) > 1);

	nodos++;										/* incrementamos aqui tambien el numero de nodos totales */
	if ((nodos % cknodos) == 0) 	        		/* comprueba cada un cierto n�mero de nodos que tenemos tiempo */
		checkup();
	if (para_busqueda == VERDADERO)					/* si hemos consumido el tiempo salimos */
		return 0;

	/* si la posicion se repite evalua como tablas (no las reclama) */
	if (repeticion () > 1)
		return 0;
	/* si hemos alcanzado una profundidad elevada salimos retornando el valor de la evaluacion */
	if (ply >= MAX_PROFUNDIDAD-1)
		return Evaluar(alpha, beta);

#if defined (UNDER_ELO)
	/* para la version limitada en fuerza y jugando con elo o nivel facil tenemos que regular el numero de nodos por segundo */
	regular_nps();
#endif

	/* comprobamos si estan cargadas las bitbases de Scorpio y las tablebases de Gaviota */
	if (egtb_is_loaded && nodo_pv) {
		total_piezas = peonesblancos + caballosblancos + alfilesblancos + torresblancas
						+ damasblancas + peonesnegros + caballosnegros + alfilesnegros + torresnegras + damasnegras + 2;
		if (total_piezas <= egtb_men) {
			int score;
			if (probe_tablebases(&score, (alpha <= -MATE+ply || beta >= MATE-ply) ? DTM_soft : WDL_soft ) == 1) {
				if (score > 0)	score = score - ply;
				else if (score < 0)	score = score + ply;
				return score;
			}
		}
	}
	pv_longuitud[ply] = ply;

	jaque = EstaEnJaque(turno);							/* comprobamos si estamos en jaque */

	/* si estamos en jaque deberiamos generar evasiones, pero como no tenemos un generador dedicado para evasiones generaremos todos
	los movimientos posibles */
	if (jaque) {
		contar2 = Generar(lista2);
	}
	/* si no estamos en jaque podremos consultar la evaluacion de la posicion */
	else {
		evalua_estatica = Evaluar(alpha, beta);
		mejor = evalua_estatica;
		alpha = MAX(alpha, mejor);
      	if (alpha >= beta) return alpha;

		/* delta prunning nos permitira cortar movimientos que no mejoran la situacion */
		material = piezasblanco+piezasnegro+peonesblanco+peonesnegro;
		if (material > MEDIOJUEGO)
			delta = alpha - 225 - evalua_estatica;
		else if (material > FINAL)
			delta = alpha - 500 - evalua_estatica;
		else
			delta = 0;

		contar2 = Generar_Capturas(lista2);
	}

	/* no tenemos movimiento de la tabla hash pero si ordenara como mejor el mejor movimiento de la anterior iteracion */
	Ordena_hmove_pv(contar2, lista2, hmov);

    /* recorremos toda la lista de movimientos */
	for (i = 0; i < contar2; ++i) {
		/* Otenemos el movimiento con mejor valor para ordenacion */
		OrdenaMovimiento(i, contar2, lista2);
		/* si no estamos en jaque y el movimiento no es una promocion */
		if (!jaque && lista2[i].tipo < PROMOCION_A_DAMA) {
			/* cortamos si el movimiento es una mala captura, si el valor de la pieza capturada es menor que delta o si el valor
			see del movimiento es menor que cero */
			if (lista2[i].valor == -1 || valor_pieza[pieza[lista2[i].a]] < delta || see2(turno, lista2[i].a, lista2[i].de) < 0)
				continue;
		}
		/* previene la explosion de la quiesce */
		if (depth <= QS_LIMITE && !jaque)
        	puntos = evalua_estatica + see(turno, lista2[i].a, lista2[i].de);
		else {
			if (!Hacer_Movimiento(lista2[i]))
				continue;
			legal++;
			puntos = -Extiende_capturas(-beta, -alpha, depth-1);
			Deshacer();
		}

		/* Salimosa si se ha acabado el tiempo */
		if (para_busqueda == VERDADERO)
			return 0;

		/* si el movimiento mejora una situacion anterior */
		if (puntos > mejor) {
			mejor = puntos;
			if (puntos > alpha) {
				alpha = puntos;
				Actualiza_pv(lista2[i]);            /* actualizamos la variante principal */
				if (puntos >= beta)
					return mejor;					/* cutoff podemos salir de la quiesce */
                //Actualiza_pv(lista2[i]);            /* actualizamos la variante principal */
			}
		}
	}

	/* si estamos en jaque y no hay movimnientos legales retornamos un MATE y distancia a la raiz */
	if (jaque && legal == 0)
		return -MATE + ply;

	return mejor;
}

/* ordena el movimiento hash en primer lugar, luego el movimiento que sigue la variante principal anterior */
void Ordena_hmove_pv(int cuenta, movimiento *lista, movimiento hmove)
{
	int i;

	if (sigue_Pv)  {
		sigue_Pv = FALSO;
		for(i = 0; i < cuenta; ++i) {
			if (lista[i].de == pv[0][ply].de && lista[i].a == pv[0][ply].a) {
				sigue_Pv = VERDADERO;
				lista[i].valor += 20000000;
				break;
			}
		}
	}
	for(i = 0; i < cuenta; ++i) {
		if (hmove.de == lista[i].de && hmove.a == lista[i].a) {
			lista[i].valor += 10000000;
			break;
		}
	}
}

/* comprueba si una posici�n se repite */
int repeticion(void)
{
	int i;
	int r = 0;

	for (i = njugadas-regla50; i < njugadas; ++i)
		if (jugadas[i].hash == hash)
			++r;
	return r;
}

/* comprueba cada cierto numero de nodos si podemos salir de la busqueda bien se haya acabado el tiempo, analisis de la posicion o
tengamos que dejar de ponderar */
void checkup(void)
{
#if defined (UNDER_CE)
	char Line[80];
#else
	char inputline[80];
#endif

	int tiempo_usado;

	/* tiempo que llevamos usado en la busqueda */
	tiempo_usado = get_ms() - tiempo_inicio;

	/* si estamos durante una busqueda y podemos acortar */
	if (estado == BUSCANDO && aborta) {
		/* si ya hemos sobrepasado el tiempo limite simplemente debemos salir de la busqueda */
		if (tiempo_usado >= limite_tiempo) {
			para_busqueda = VERDADERO;
			return;
		}
		/* si ya no temos tiempo para buscar un nuevo movimiento y la evaluacion en el actual ply no se ha reducido por un cierto margen
		debemos cortar la busqueda */
		//if (tiempo_usado > no_nuevo_movimiento && puntos + MARGEN >= valor[li-1]) {
		if (!ply && tiempo_usado > no_nuevo_movimiento && fail_low < 3 && fail_high < 3) {
            para_busqueda = VERDADERO;
			return;
		}
		/* si recibimos algo de la GUI */
		if (bioskey()) {
			/* obtenemos el comando enviado por la GUI */
#if defined (UNDER_CE)
			/* PocketPc */
			if (gets(Line) == NULL)
			return;
			sscanf(Line, "%s", inputline);
#else
			/* resto */
			readline(inputline, sizeof(inputline));
#endif
			/* si el comando es quit debemos abandonar la busqueda y hasta salir del motor */
			if (strcmp(inputline, "quit") == 0) {
				printf("Quitting during think, bye\n");
				exit(0);
			}
			/* si estamos en el protocolo xboard podemos esperar un comando ? que nos haga terminar la busqueda */
			if (protocolo == XBOARD) {
				if (strcmp(inputline, "?") == 0) {
					para_busqueda = VERDADERO;
					motor = VACIO;
					return;
				}
			}
			/* si estamos en el protocolo uci podemos esperar un comando stop para terminar la busqueda, el comando isready no sera
			habitual */
			else {
				if (strcmp(inputline, "stop") == 0) {
					para_busqueda = VERDADERO;
					return;
				}
				if (strcmp(inputline, "isready") == 0) {
					printf("readyok\n");
					return;
				}
			}
			return;
		}
	}
	/* si estamos en el proceso de ponderar */
	else if (estado == PONDERANDO) {
		/* comprobamos si recibimos algo de la GUI */
		if (bioskey()) {
			/* si estamos en el protocolo xboard es se�al directa de parar el pensamiento */
			if (protocolo == XBOARD) {
				para_busqueda = VERDADERO;
				return;
			}
			/* si estamos en el protocolo uci */
			else {
				/* obtenemos el comando */
				readline(inputline, sizeof(inputline));
				/* el comando quit haria terminar al motor */
				if (strcmp(inputline, "quit") == 0) {
					printf("Quitting during ponder, bye\n");
					exit(0);
				}
				/* el comando stop hace terminar el ponder y empezar un nueva busqueda */
				if (strcmp(inputline, "stop") == 0) {
					para_busqueda = VERDADERO;
					return;
				}
				/* el comando ponderhit haria que el motor siguiera pensando pero cambiado el estado de ponderando a buscando */
				if (strcmp(inputline, "ponderhit") == 0) {
					/* si estamos en un modo de profundidad fija o tiempo por movimiento */
					if (pdm) {
						/* recuperamos valores indicados por la GUI cuando empezamos a ponderar */
						max_profundidad = guarda_profundidad;
						tiempo_total = guarda_tiempo;
						inc = guarda_incremento;
						quedan = guarda_quedan;
						no_nueva_iteracion = tiempo_total;
						no_nuevo_movimiento = tiempo_total;
						limite_tiempo = tiempo_total;
					}
					/* si tenemos un tiempo para el juego */
					else {
						/* recuperamos valores indicados por la GUI cuando empezamos a ponderar y utilizaremos el mismo tipo de gestion
						para el tiempo como cuando hacemos una busqueda */
						max_profundidad = guarda_profundidad;
						tiempo_total = guarda_tiempo/2;						/* con un ponderhit utilizamos solo la mitad del tiempo */
						inc = guarda_incremento;
						quedan = guarda_quedan;
						if (inc > 0 && tiempo_total < 10*inc) {
							tiempo_total = 75*inc/100;
							no_nueva_iteracion = tiempo_total/2;
							no_nuevo_movimiento = tiempo_total;
							limite_tiempo = tiempo_total;
						}
						else {
							no_nueva_iteracion = ((tiempo_total/2) / quedan) + inc;
							no_nuevo_movimiento = ((3*tiempo_total/2) / (quedan + (1/2))) + inc;
							limite_tiempo = ((10*tiempo_total) / (quedan + 9)) + inc;
						}
					}
					tiempo_inicio = get_ms();								/* tenemos que resetear el contador del tiempo */
					aborta = VERDADERO;
					estado = BUSCANDO;
					return;
				}
				/* no sera normal que recibamos el comando isready */
				if (strcmp(inputline, "isready") == 0) {
					printf("readyok\n");
					para_busqueda = FALSO;
					return;
				}
			}
			return;
		}
	}
	/* si estamos en el modo de analizar */
	else if (estado == ANALIZANDO) {
		/* comprobamos que recibimos algo de la GUI y obtenemos el comando que recibimos */
		if (bioskey()) {
#if defined (UNDER_CE)
			/* PocketPc */
			if (gets(Line) == NULL)
			return;
			sscanf(Line, "%s", inputline);
#else
			/* resto */
			readline(inputline, sizeof(inputline));
#endif
			/* si recibmos un comando quit debe terminar la ejecucion del motor */
			if (strcmp(inputline, "quit") == 0) {
				printf("Quitting during analyze, bye\n");
				exit(0);
			}
			/* si estamos en el protocolo xboard */
			if (protocolo == XBOARD) {
				/* si recibimnos un comando . no hacemos caso */
				if (strcmp(inputline, ".") == 0) {
					return;
				}
				/* con el comando exit termina la busqueda y por tanto el analisis */
				if (strcmp(inputline, "exit") == 0) {
					para_busqueda = VERDADERO;
					return;
				}
			}
			/* si estamos en el protocolo uci */
			else {
				/* con el comando stop debemos para el analisis */
				if (strcmp(inputline, "stop") == 0) {
					para_busqueda = VERDADERO;
					return;
				}
				/* no sera habitual recibir el comando isready */
				if (strcmp(inputline, "isready") == 0) {
					printf("readyok\n");
					return;
				}
			}
			return;
		}
		/* terminamos si hemos sobrepasado el tiempo limite aunque en la practica no ocurrira ya que es casi infinito */
		if (tiempo_usado >= limite_tiempo) {
			para_busqueda = VERDADERO;
			return;
		}
	}
	return;
}

#if defined (UNDER_ELO)
/* en la version limitada de fuerza reduce el nps para adaptar el elo */
void regular_nps(void)
{
	int tiempo_gastado;
	double nps;

	tiempo_gastado = get_ms() - tiempo_inicio;
	if (tiempo_gastado > 0) {
		nps = nodos / tiempo_gastado * 1000;
		while (nps > nps_elo) {
			Sleep(1);
			tiempo_gastado = get_ms() - tiempo_inicio;
			nps = nodos / tiempo_gastado * 1000;
		}
	}
}
#endif
