#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "linux-functions.h"
#define line_size 80
struct instrument {
	char *type;
	int num;
	char *lname;
	char *fname;
	bool out;
};
void blank(const int length, void **array)
{
	for (int i = 0; i < length; i++)
		array[i] = 0;
	return;
}
void clearline(FILE *input)
{
	int c;
	do
		c = fgetc(input);
	while (c != '\n' && c != -1);
	return;
}
void println(struct instrument *in, FILE *printdest)
{
	fprintf(printdest, "%s\t%d\t%s\t%s\n", in->type, in->num, in->lname, in->fname);
	return;
}
void printarr(struct instrument **arr, FILE *printdest, const int which) //0 for checked in, 1 for checked out, 2 for all
{
	for (int i = 0; i < 30; i++)
	{
		if (arr[i] == 0)
			return;
		if (arr[i]->out == which || which == 2)
			println(arr[i], printdest);
	}
	return;
}
void addtoarray(struct instrument **array, struct instrument *add)
{
	int i = 0;
	while (array[i] != 0 && i < 30)
	{
		//Check to see if the same instrument is already checked out
		if (array[i]->num == add->num)
		{
			int errcorrectint = 0; struct instrument *errcorrectstruct;
			while (errcorrectint > 2 || errcorrectint < 1)
			{
				printf("Two instruments exist with the same number %d.\n1)\t", add->num);
				println(add, stdout); printf("2)\t"); println(array[i], stdout);
				printf("Which is correct? ");
				switch (getchar())
				{
				case '1':
					errcorrectint = 1;
					break;
				case '2':
					errcorrectint = 2;
					break;
				default:
					printf("I'm sorry, your input wasn't understood. Please try again.\n");
					break;
				}
				clearline(stdin);
			}
			if (errcorrectint == 1)
				errcorrectstruct = add;
			else
				errcorrectstruct = array[i];
			printf("What number should \n   "); println(errcorrectstruct, stdout); printf("   be corrected to be? (!!Caution: any number will be accepted, doesn't check for non-numbers!!)");
			char *errcorrectstring = malloc(80 * sizeof(char));
			size_t n = 80;
			getline(&errcorrectstring, &n, stdin);
			errcorrectstruct->num = atoi(errcorrectstring);
			free(errcorrectstring);
		}
		i++;
	}
	if (i >= 30)
	{
		fprintf(stderr, "**** ERR: OUT OF MEMORY FOR MORE INSTRUMENTS OF TYPE %s ****", add->type);
		free(add->type);
		free(add->lname);
		free(add->fname);
		free(add);
	}
	else
	{
		array[i] = add;
	}
	return;
}
struct instrument *locateinarray(int num, struct instrument **array)
{
	//Locate an instrument of a given number in an array
	for (int i = 0; i < 30 && array[i] != 0; i++)
	{
		if (array[i]->num == num)
			return array[i];
	}
	//If reaches here, none match in array.
	return 0;
}
long updatefile(FILE *stream, size_t overwrite, size_t size, char * buffer, long pos)
{
	//Shuffle bytes around before adding
	fflush(stream);
	long size_change = size - overwrite;
	long resume = ftell(stream);
	/* Position resume so it stays in the same relative position to
	 * everything being moved. If locatin is in middle of deleted area,
	 * position the pointer at the start of area to be deleted */
	if (resume >= pos)
	{
		resume = resume + size_change;
		if (resume < pos)
			resume = pos;
	}
	/************ Diagram of memory ************
	     |------------- size --------------|
	     |-- overwrite --|-- size_change --|
	          Need to add bytes overall
	             ****    OR    ****
	     |---------- overwrite ------------|
	     |--- size ---|--- size_change ----|
	          Need to remove bytes overall
	          Deletion of overwrite if size = 0
	*******************************************/
	if (size_change != 0)
	{
		fseek(stream, pos + overwrite, SEEK_SET);
		char *change1 = malloc(1024 * sizeof(char)), *change2;
		int c, bytes_read1 = 0, bytes_read2 = 0;
		/* Read in next KiB of file after overwrite, stopping if EOF is
		 * reached or 1 KiB has been read */
		long KiBstart1 = ftell(stream), KiBstart2 = 0;
		while ((c = fgetc(stream)) != EOF && bytes_read1 < 1024)
		{
			change1[bytes_read1] = 0;
			bytes_read1++;
		}
		/* For the rest of the file, read in 1KiB as before, back up to
		 * the start of the KiB, and write out however much was read.
		 * If EOF was reached, loop will be skipped, and writing out
		 * will continue after the loop, with correct amount read */
		while (c != EOF)
		{
			KiBstart2 = ftell(stream);
			bytes_read2 = 0;
			change2 = malloc(1024 * sizeof(char));
			while ((c = fgetc(stream)) != EOF && bytes_read2 < 1024)
			{
				change2[bytes_read2] = 0;
				bytes_read2++;
			}
			fseek(stream, KiBstart1 + size_change, SEEK_SET);
			for (int i = 0; i < bytes_read1; i++)
			{
				fputc(change1[i], stream);
			}
			free(change1);
			change1 = change2;
			bytes_read1 = bytes_read2;
			KiBstart1 = KiBstart2;
		}
		/* Seek back to start of last KiB and write out characters that
		 * have been read. */
		fseek(stream, KiBstart1 + size_change, SEEK_SET);
		for (int i = 0; i < bytes_read1; i++)
		{
			fputc(change1[i], stream);
		}
		free(change1);
	}
	//File bytes moved so overall change in length
	fseek(stream, pos, SEEK_SET);
	for (int i = 0; i < size; i++)
	{
		fputc(buffer[i], stream);

	}
	fseek(stream, resume, SEEK_SET);
	return resume;
}
int readentry(struct instrument *add, char *line) //1 normal, -1 empty line
{
	//TODO fix this function. It doesn't work.
	//Empty and comment lines must already be removed
	//int linelength = strlen(line) + 2;
	char *filelinesplit = strtok(line, "\n\t");
	for (int column = 1; column < 5; column++)
	{
		char *linecopy = malloc((strlen(filelinesplit) + 2) * sizeof(char));
		strcopy(linecopy,filelinesplit);
		if (linecopy[0] == 0)
		{
			free(linecopy);
			if (column == 1)
				return -1;
			if (column == 2)
			{
				free(add->type);
				return -1;
			}

		}

		filelinesplit = strtok(NULL, "\t\n");
	}
}
int main(void)
{

	printf("The outtab file must be located in the same directory as the program, as it stores data about who has checked out what.");
	FILE *outtab = fopen("outtab", "r+");
	rewind(outtab);
	struct instrument *vln[30], *vla[30], *clo[30], *bas[30];
	blank(30, vla); blank(30, vln); blank(30, clo); blank(30, bas);
	if (outtab == 0)
	{
		printf("**** ERR: UNABLE TO OPEN \"outtab\" FOR READING. ****\nEnsure the program is located in the same folder as \"outtab\"");
		exit(1);
	}

	//Import list
	//ssize_t getline(char **lineptr, size_t *n, FILE *stream);
	int linenum = 0;
	while (true)
	{
		struct instrument *temptostore = malloc(sizeof(struct instrument));
		temptostore->out = true; temptostore->lname = 0; temptostore->fname = 0; temptostore->type = 0; temptostore->num = 0;
		char *fileline = malloc(80 * sizeof(char));
		int filelinechars, filelinelength = 80;
		filelinechars = getline(&fileline, &filelinelength, outtab);
		linenum++;
		if (filelinechars == EOF || fileline[0] == '#' || fileline[0] == '\n')
		{
			//free(fileline);
			free(temptostore);
		}
		else
		{
			//Divides outtab by \t, loading into temptostore
			char *filelinebegin = fileline, *filelinesplit = strtok(fileline, "\t\n");
			for (int column = 1; column < 5; column++) //Loop counts column of outtab
			{
				char *linecopy = malloc(filelinechars * sizeof(char));
				strcopy(linecopy,filelinesplit);
				//If splitting results in nothing, mark entry as checked out or discard line
				if (filelinesplit[0] == '\0')
				{
					if (column > 1)
					{
						temptostore->out = false;
					}
					else if (column = 1)
					{
						free(temptostore->type);
					}
					free(linecopy);
					column = 7;
					break;
				}
				//Switch based on order in outtab
				switch (column)
				{
					case 1:
						temptostore->type = linecopy;
						break;
					case 2:
						temptostore->num = atoi(linecopy);
						//Ensure sane instrument numbers (ie > 0)
						if (temptostore->num < 1)
						{
							fprintf(stderr,"**** ERR: ABNORMAL INSTRUMENT NUMBER - LINE %d ****", linenum);
							free(temptostore->type);
							free(temptostore);
						}
						free(linecopy);
						break;
					case 3:
						temptostore->lname = linecopy;
						break;
					case 4:
						temptostore->fname = linecopy;
						break;
					case 7:
						break;
					default:
						fprintf(stderr, "**** ERR: SPLITTING STRING - LINE %d ****", linenum);
						break;
				}
				filelinesplit = strtok(NULL, "\t\n");
			}
			if (strcmp("vln", temptostore->type) == 0)
				addtoarray(vln, temptostore);
			else if (strcmp("vla", temptostore->type) == 0)
				addtoarray(vla, temptostore);
			else if (strcmp("clo", temptostore->type) == 0)
				addtoarray(clo, temptostore);
			else if (strcmp("bas", temptostore->type) == 0)
				addtoarray(bas, temptostore);
			else
			{
				fprintf(stderr, "**** ERR: INSTRUMENT TYPE NOT FOUND: %s - LINE %d ****", temptostore->type, linenum);
				free(temptostore->type);
				free(temptostore->lname);
				free(temptostore->fname);
				free(temptostore);
			}
			free(filelinebegin);
		}
	}
	//fclose(outtab);

	//Make changes to check in/out lists
	fprintf(stdout, "The current checkouts are as follows:\n");
	printarr(vln, stdout, true); printarr(vla, stdout, true); printarr(clo, stdout, true); printarr(bas, stdout, true);
	fprintf(stdout, "The current checkins are as follows:\n");
	printarr(vln, stdout, false); printarr(vla, stdout, false); printarr(clo, stdout, false); printarr(bas, stdout, false);

	bool contloop = true, save = true;
	while (contloop)
	{

		printf("Checkout (o), Checkin (i), Reprint (r), Quit (q), Advanced (~)");
		switch(getchar())
		{
			case 'o':
			case 'O':
				struct instrument *temptoinsert = malloc(sizeof(struct instrument));
				clearline(stdin); //Ensure stdin has no trailing newline
				printf("Instrument type and num to check out");
				char *fileline = malloc(80 * sizeof(char));
				ssize_t filelinechars;
				size_t filelinelength = 80;
				filelinechars = getline(&fileline, &filelinelength, stdin);
				if (filelinechars == -1) //If something wrong happened, notify user and break switch
				{
					printf("Something has gone wrong reading the line, please try again.");
					break;
				}
				char *filelinesplit = strtok(fileline, "\t\n");
				struct instrument **array = 0;
				//Find correct array - Braces to be able to collapse section
				{
					//Determine array the instrument needs to go into
					if (strcmp(filelinesplit, "vln") == 0)
						array = vln;
					else if (strcmp(filelinesplit, "vla") == 0)
						array = vla;
					else if (strcmp(filelinesplit, "clo") == 0)
						array = clo;
					else if (strcmp(filelinesplit, "bas") == 0)
						array = bas;
					else
					{
						printf("Instrument type not found: %s\n", filelinesplit);
						break;
					}
				}
				filelinechars = strtok(NULL, "\t\n");
				int instrumentnum = atoi(filelinechars);

				char *linecopy = malloc(filelinechars * sizeof(char));

		}

	}
}
/*
	Import whole table into array 30 long
	Prompt for deletion or additon of persons. Print whole list.
		Deletion by vla 3
		Additon by fname, lname
	Update outtab upon new entries or deletions.
*/
