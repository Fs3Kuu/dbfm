/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * tn.razy@gmail.com
 */

#include "log.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

struct looparg
{
	void (*callback)(void *);
	void *data;
	int nsec;
};

static inline void *loop(void *arg);

void duration(void *data, void (*func)(void *), int nsec)
{
	struct looparg *arg = calloc(sizeof *arg, 1);

	arg->nsec = nsec;
	arg->callback = func;
	arg->data = data;

	pthread_t thread;
	pthread_create(&thread, NULL, loop, arg);
}

void prompt(const char *text, char *save, int size, enum input_type type)
{
	fprintf(stderr, "\n%s", text ? text : "");

	fflush(stderr);

	if(size <= 0)
	{
		return;
	}

	if(type < INPUT_TEXT || type >= INPUT_UNKNOW)
	{
		type = INPUT_TEXT;
	}

	fflush(stdin);

	if(INPUT_PASSWORD == type)
	{
		echo(0);
	}
	else
		echo(1);

	register int ch = 0, n = 0;

	while(!feof(stdin) && --size > 0)
	{
		ch = getchar();

		if(10 == ch)
		{
			break;
		}
		
		*(save + n++) = ch;
	}

	*(save + n) = 0;

	echo(1);
}

void echo(int enable)
{
	struct termios term;

	tcgetattr(STDIN_FILENO, &term);

	term.c_lflag = enable ? term.c_lflag | ICANON | ECHO : term.c_lflag & ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

static inline void *loop(void *data)
{
	pthread_cleanup_push(free, data);

	struct looparg *arg = (struct looparg *)data;

	int nsec = arg->nsec;

	while(1)
	{
		sleep(nsec < 0 ? 30 : nsec);
		
		(arg->callback)(arg->data);
	}

	pthread_cleanup_pop(0);

	return NULL;
}

char *meta(const struct hash **track, const char *fmt, char *buf)
{
	char *ptr = (char *)fmt;
	int remsize = BUFSIZ;

	memset(buf, 0, BUFSIZ);

	register char ch = 0;

	while(*ptr && remsize - 2 > 0)
	{
		ch = *ptr;

		if('%' == ch)
		{
			const char *v = NULL;
			int width = 0;

			if(isdigit(*++ptr))
			{
				width = atoi(ptr);

				while(isdigit(*++ptr));
			}

			switch(*ptr)
			{
				case 'a':
					v = value(track, "artist");
					break;
				case 't':
					v = value(track, "title");
					break;
				case 'b':
					v = value(track, "albumtitle");
					break;
				case 'l':
					v = value(track, "length");
					break;
				case 'y':
					v = value(track, "public_time");
					break;

				default:
					buf[BUFSIZ - remsize--] = ch;
					buf[BUFSIZ - remsize] = *ptr;
					continue;
			}

			if(v && (remsize -= strlen(v)) > 0)
			{
				strcat(buf, v);
				
				if(width > strlen(v))
				{
					int rem = width - strlen(v);

					while(--rem >= 0)
					{
						strcat(buf, " ");

						--remsize;
					}
				}

				++ptr;
				continue;
			}
		}

		buf[BUFSIZ - remsize--] = ch;
		buf[BUFSIZ - remsize] = 0;

		++ptr;
	}

	return buf;
}

int arridx(const char **arr, char *value)
{
	for(int i = 0; arr[i]; i++)
	{
		if(strlen(value) == strlen(arr[i]) && (0 == strcasecmp(arr[i], value)))
		{
			return i;
		}
	}

	return -1;
}
