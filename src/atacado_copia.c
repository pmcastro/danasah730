/*
	< DanaSah is a winboard/xboard and uci chess engine. >
    Copyright (C) <2016>  <Pedro Castro Elgarresta>
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
La función principal aqui va a ser comprobar si una casilla es atacada por una pieza y en base a ello comprobamos si el rey está en jaque,
la función atacado esta basado en las características de generación de movimientos de FirstChess, el generador más simple posible, el
codigo seria mas sencillo si usamos mailbox pero no tendriamos ventajas en cuanto a velocidad
*/

/*Includes*/
#include "definiciones.h"
#include "variables.h"
#include "funciones.h"

/* comprueba si el rey esta en jaque, tenemos guardadas la posición de los reyes rb (rey blanco) y rn ya que vamos actualizando su
posicion en las funciones hacer_movimiento() y deshacer() */
int	EstaEnJaque(int turno2)
{
	if (turno2 == BLANCO)
		return atacado(rb, BLANCO);
	else
		return atacado(rn, NEGRO);
}

/* comprueba si una casilla k es atacada por una casilla de color contraria al turno que toca, retorna 1 (VERDADERO) si es atacada y
un 0 (FALSO) si no lo es, esta función la vamos a utilizar para saber si el rey está en jaque y tambien nos permitira controlar la
seguridad del rey sabiendo si una casilla cercana al rey está siendo atacada */
int atacado(int sq, int turno2)
{
	int i, n, sq_o, s;
	register int ndir;

	s = 1 - turno2;

	/* bishop style-move */
	for (i = 0; i < 4; i++) {
		ndir = offset[ALFIL][i];
		sq_o = mailbox64[sq]+ndir;
		n = mailbox[sq_o];
		if (n == -1) continue;
		/* attack by pawns */
		if (pieza[n] == PEON && color[n] == s) {
			if (s == BLANCO && (i >= 2)) return VERDADERO;
			if (s == NEGRO && (i <= 1)) return VERDADERO;
		}
		/* attack by king */
		if (/*k == 1 && */pieza[n] == REY && color[n] == s) return VERDADERO;
		/* slide */
		do  {
			if ((pieza[n] == ALFIL || pieza[n] == DAMA) && color[n] == s) return VERDADERO;
			if (color[n] != VACIO) break;
			sq_o += ndir;
			n = mailbox[sq_o];
		} while (n != -1);
	}
	/* knight style-move */
	for (i = 0; i < 8; i++) {
		n = mailbox[mailbox64[sq]+offset[CABALLO][i]];
		if (pieza[n] == CABALLO && color[n] == s) return VERDADERO;
	}
	/* rook style-move */
	for (i = 0; i < 4; i++) {
		ndir = offset[TORRE][i];
		sq_o = mailbox64[sq]+ndir;
		n = mailbox[sq_o];
		if (n == -1) continue;
		if (pieza[n] == REY && color[n] == s) return VERDADERO;
		do  {
			if ((pieza[n] == TORRE || pieza[n] == DAMA) && color[n] == s) return VERDADERO;
			if (color[n] != VACIO) break;
			sq_o += ndir;
			n = mailbox[sq_o];
		} while (n != -1);
	}
	/* no attack */
	return FALSO;
}

/* ahora vamos a tener una variante de la funcion atacado, en este caso es para utilizarla en la seguridad del rey y saber si una casilla
cercana al rey está defendida por las piezas del mismo color excepto por el propio rey */
int atacado_nr(int k, int turno2)
{
	int				h,
                    y,
                    fila,
                    columna,
                    cturno;

	cturno = (BLANCO + NEGRO) - turno2;
    fila = FILA(k), columna = COLUMNA(k);

    /* Chequea los ataques del caballo */
    if (columna > 0 && fila > 1 && color[k - 17] == cturno && pieza[k - 17] == CABALLO)
        return 1;
    if (columna < 7 && fila > 1 && color[k - 15] == cturno && pieza[k - 15] == CABALLO)
        return 1;
    if (columna > 1 && fila > 0 && color[k - 10] == cturno && pieza[k - 10] == CABALLO)
        return 1;
    if (columna < 6 && fila > 0 && color[k - 6] == cturno && pieza[k - 6] == CABALLO)
        return 1;
    if (columna > 1 && fila < 7 && color[k + 6] == cturno && pieza[k + 6] == CABALLO)
        return 1;
    if (columna < 6 && fila < 7 && color[k + 10] == cturno && pieza[k + 10] == CABALLO)
        return 1;
    if (columna > 0 && fila < 6 && color[k + 15] == cturno && pieza[k + 15] == CABALLO)
        return 1;
    if (columna < 7 && fila < 6 && color[k + 17] == cturno && pieza[k + 17] == CABALLO)
        return 1;

	/* Chequea las lineas horizontales y verticales para los ataques de dama, torre */
    y = k + 8;
    if (y < 64) {
        if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == TORRE))
            return 1;
        if (pieza[y] == VACIO)
            for (y += 8; y < 64; y += 8) {
                if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == TORRE))
                    return 1;
                if (pieza[y] != VACIO)
                    break;
            }
    }

    y = k - 1;
    h = k - columna;
    if (y >= h) {
        if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == TORRE))
            return 1;
        if (pieza[y] == VACIO)
            for (y--; y >= h; y--) {
                if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == TORRE))
                    return 1;
                if (pieza[y] != VACIO)
                    break;
            }
    }

    y = k + 1;
    h = k - columna + 7;
    if (y <= h) {
        if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == TORRE))
            return 1;
        if (pieza[y] == VACIO)
            for (y++; y <= h; y++) {
                if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == TORRE))
                    return 1;
                if (pieza[y] != VACIO)
                    break;
            }
    }

    y = k - 8;
    if (y >= 0) {
        if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == TORRE))
            return 1;
        if (pieza[y] == VACIO)
            for (y -= 8; y >= 0; y -= 8) {
                if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == TORRE))
                    return 1;
                if (pieza[y] != VACIO)
                    break;
            }
    }

	/* Chequea las diagonales para los ataques de dama, alfil y peon */
    y = k + 9;
    if (y < 64 && COLUMNA(y) != 0) {
        if (color[y] == cturno) {
            if (pieza[y] == DAMA || pieza[y] == ALFIL)
                return 1;
            if (turno2 == NEGRO && pieza[y] == PEON)
                return 1;
        }
        if (pieza[y] == VACIO)
            for (y += 9; y < 64 && COLUMNA(y) != 0; y += 9) {
                if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == ALFIL))
                    return 1;
                if (pieza[y] != VACIO)
                    break;
            }
    }

    y = k + 7;
    if (y < 64 && COLUMNA(y) != 7) {
        if (color[y] == cturno) {
            if (pieza[y] == DAMA || pieza[y] == ALFIL)
                return 1;
            if (turno2 == NEGRO && pieza[y] == PEON)
                return 1;
        }
        if (pieza[y] == VACIO)
            for (y += 7; y < 64 && COLUMNA(y) != 7; y += 7) {
                if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == ALFIL))
                    return 1;
                if (pieza[y] != VACIO)
                    break;
            }
    }

    y = k - 9;
    if (y >= 0 && COLUMNA(y) != 7) {
        if (color[y] == cturno) {
            if (pieza[y] == DAMA || pieza[y] == ALFIL)
                return 1;
            if (turno2 == BLANCO && pieza[y] == PEON)
                return 1;
        }
        if (pieza[y] ==	VACIO)
            for (y -= 9; y >= 0 && COLUMNA(y) != 7; y -= 9) {
                if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == ALFIL))
                    return 1;
                if (pieza[y] != VACIO)
                    break;
            }
    }

    y = k - 7;
    if (y >= 0 && COLUMNA(y) != 0) {
        if (color[y] == cturno) {
            if (pieza[y] == DAMA || pieza[y] == ALFIL)
                return 1;
            if (turno2 == BLANCO && pieza[y] == PEON)
                return 1;
        }
        if (pieza[y] == VACIO)
            for (y -= 7; y >= 0 && COLUMNA(y) != 0; y -= 7) {
                if (color[y] == cturno && (pieza[y] == DAMA || pieza[y] == ALFIL))
                    return 1;
                if (pieza[y] != VACIO)
                    break;
            }
    }
    return 0;
}
