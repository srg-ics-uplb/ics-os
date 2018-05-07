/*
    Perico Dan B. Dionisio
    Coleen Bobilles
    CMSC 125 ST-2L
*/

#include "../../sdk/dexsdk.h"
#include "../../sdk/time.h"

#define up_key 'w'
#define down_key 's'
#define right_key 'd'
#define left_key 'a'
#define enter '\n'
#define quit 27
#define yes 'y'
#define no 'n'
#define main_menu 'm'

/*
    (colors are from the blacjack application in the ics-os)
*/
#define YELLOW 54
#define GRAY 56
#define WHITE 63
#define BROWN 20
#define VIOLET 40
#define RED 36
#define GREEN 18
#define BLUE 9
#define ORANGE 38
#define MAROON 4
#define FLESH 55
#define BLACK 70
#define DARK_GREEN 16
#define DARK_BROWN 32

char player1_name[50] = "player1";
char player2_name[50] = "player2";
int player1_current = 1;
int player2_current = 15;
int game_number = 1;

int items[16] = {0, 7, 7, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 7, 7};
//int items[16] = {57, 1, 0, 0, 0, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0};
//int items[16] = {97, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*
    "Erases" the screen given the starting point and the width & height
    (from the blacjack application in the ics-os)
*/
void drawRectangle(int x, int y, int w, int h, int color){
   int i,j;
   for (i=y;i<=(y+h);i++)
      for (j=x;j<=(x+w);j++)
         write_pixel(j,i,color);
}

/*
    this function will clear the screen and display the menu
*/
void drawMenu(){
    drawRectangle(0,0,320,220, BLACK);
    write_text("SUNGKA",41,41,WHITE,1);

    write_text("[Enter] Start",40,120,WHITE,0);
	write_text("[Esc] Quit",40,140,WHITE,0);
	write_text("[i] Instructions", 40, 160, WHITE, 0);
}

/*
    this function will draw the current state of the board
*/
void drawBoard(){
    int x, z;
    char temp[1];
    //drawRectangle(0,0,320,220, BLACK);

    drawRectangle(4,70,30,20, BROWN);                                           //this will draw the head of player 1
    sprintf(temp, "%d", items[0]);
    write_text(temp,12,75,WHITE,0);

    z=1;
    for(x=39; x<270; x+=35){                                                    //the houses of player 1
        if(items[(x-4)/35]!=-1){
            drawRectangle(x,40,30,20, BROWN);
            sprintf(temp, "%d", items[(x-4)/35]);
            write_text(temp,x+8,45,WHITE,0);
        }
        else{
            drawRectangle(x,40,30,20, GRAY);
        }

    }

    z=8;
    for(x=39; x<270; x+=35){                                                    //the houses of player 2
        if(items[(16-(x-4)/35)]!=-1){
            drawRectangle(x,100,30,20, DARK_BROWN);
            sprintf(temp, "%d", items[(16-(x-4)/35)]);
            write_text(temp,x+8,105,WHITE,0);
        }
        else{
            drawRectangle(x,100,30,20, GRAY);
        }
    }

    drawRectangle(285,70,30,20, DARK_BROWN);                                    //the head of player 2
    sprintf(temp, "%d", items[8]);
    write_text(temp,293,75,WHITE,0);


}

/*
    to display the current game number in the main board
*/
void drawGameNumber(){
    char temp2[20];
    drawRectangle(130,140,90,180, BLACK);
    sprintf(temp2, "Game %d", game_number);
    write_text(temp2,130,140,WHITE,1);

    write_text("[a] Move Left",10,160,WHITE,0);
    write_text("[d] Move Right",150,160,WHITE,0);
    write_text("[enter] Select",10,180,WHITE,0);
    write_text("[esc] Exit",150,180,WHITE,0);
}

/*
    to highlight the current cell where there will be an update
*/
void highlight(int a){
    char temp[1];
    if(a == 0){
        drawRectangle(4,70,30,20, FLESH);
        sprintf(temp, "%d", items[0]);
        write_text(temp,12,75,BLACK,0);
    }
    else if(a > 0 && a < 8){
        drawRectangle(a*35+4,40,30,20, FLESH);
        sprintf(temp, "%d", items[a]);
        write_text(temp,a*35+12,45,BLACK,0);
    }
    else if ( a == 8 ){
        drawRectangle(285,70,30,20, FLESH);
        sprintf(temp, "%d", items[8]);
        write_text(temp,293,75,BLACK,0);
    }
    else {
        drawRectangle(a*-35+564,100,30,20, FLESH);
        sprintf(temp, "%d", items[a]);
        write_text(temp,a*-35+572,105,BLACK,0);
    }
}

/*
    to draw a triangle above the current cell
*/
void drawInvertedTriangle(int x, int y, int w, int h, int color){
    int i,j;
    for (i=y;i<=(y+h);i++)
       for (j=x+i-y;j<=(x+w)-i+y;j++)
          write_pixel(j,i,color);
}

/*
    to print whose turn it is
*/
void printTurn(int turn){
    char temp[100];
    drawRectangle(60, 70, 225, 20, BLACK);
    if(turn==1)
        strcpy(temp, player1_name);
    else
        strcpy(temp, player2_name);
    strcat(temp, "'s turn");
    write_text(temp,60,70,WHITE,0);
}

/*
    the logic of the game: moving different items between cells

*/
int moveItems(int player){
    int index;
    int item;
    if(player==1){
        if(items[player1_current]<=0){                                          //if the selected cell contains 0 or negative 1
            return 1;
        }
        item = items[player1_current];
        items[player1_current] = 0;
        drawBoard();
        highlight(player1_current);
        for(index=player1_current-1; item>0; index--){
            index = (index+16) % 16;
            if(items[index]==-1){}
            else if(index!=8){
                item--;
                items[index] = items[index] + 1;
                if(item!=0){
                    delay(10);
                    drawBoard();
                    highlight(index);
                }
                else if(item==0 && index==0){                                   //if it ends in the head, the turn does not end
                    delay(10);
                    drawBoard();
                    return 1; //the turn does not end
                }
                else if(item==0 && items[index]>1){                             //if it ends in an non-empty cell
                    delay(10);
                    drawBoard();
                    item = items[index];
                    items[index] = 0;
                    delay(10);
                    drawBoard();
                    highlight(index);
                }
                else if(item==0 && index>=9 && index<16){                       //ends in the empty house of the opponent
                    delay(10);
                    drawBoard();
                    return 0; //end of turn
                }
                else if(item==0){                                               //if it landed in his/her empty house
                    delay(10);
                    drawBoard();
                    highlight(index);
                    items[index] = 0;
                    if(items[index+((8-index)*2)]!=-1){                         //this will get the items in the "katapat na " house
                        item = items[index+((8-index)*2)] + 1;
                        items[index+((8-index)*2)] = 0;
                        delay(10);
                        drawBoard();
                        highlight(index+((8-index)*2));
                        items[0] = item + items[0];
                        delay(10);
                        drawBoard();
                        highlight(0);
                        delay(10);
                        drawBoard();
                    }
                    else{
                        items[0] = 1 + items[0];
                    }
                    delay(10);
                    drawBoard();
                    return 0; // end of turn
                }
            }
        }
    }

    if(player==2){                                                              //same logic but applied for the player 2
        if(items[player2_current]<=0){
            return 1;
        }
        item = items[player2_current];
        items[player2_current] = 0;
        drawBoard();
        highlight(player2_current);
        for(index=player2_current-1; item>0; index--){
            index = (index+16) % 16;
            if(items[index]==-1){}
            else if(index!=0){
                item--;
                items[index] = items[index] + 1;
                if(item!=0){
                    delay(10);
                    drawBoard();
                    highlight(index);
                }
                else if(item==0 && index==8){
                    delay(10);
                    drawBoard();
                    return 1; //the turn does not end
                }
                else if(item==0 && items[index]>1){
                    delay(10);
                    drawBoard();
                    item = items[index];
                    items[index] = 0;
                    delay(10);
                    drawBoard();
                    highlight(index);
                }
                else if(item==0 && index>0 && index<8){
                    delay(10);
                    drawBoard();
                    return 0; //end of turn
                }
                else if(item==0){
                    delay(10);
                    drawBoard();
                    highlight(index);
                    items[index] = 0;
                    if(items[index+((8-index)*2)]!=-1){
                        item = items[index+((8-index)*2)] + 1;
                        items[index+((8-index)*2)] = 0;
                        delay(10);
                        drawBoard();
                        highlight(index+((8-index)*2));
                        items[8] = item + items[8];
                        delay(10);
                        drawBoard();
                        highlight(8);
                        delay(10);
                        drawBoard();
                    }
                    else{
                        items[8] = 1 + items[8];
                    }
                    delay(10);
                    drawBoard();
                    return 0; // end of turn
                }
            }
        }
    }
}

/*
    this will check if the current game is finished
*/
int checkIfFinish(){
    int i=0;
    for(i=1; i<16; i++){
        if(i==8) continue;
        if(items[i]>0) return 1;                                                //not yet finish
    }
    return -1;                                                                  //move on to the next game
}

/*
    this will display the winner of the game
*/
int printWinner(int a){
    drawRectangle(0,0,320,220, BLACK);
    if(a==1){
        write_text("Player 1 Won !!!",41,41,WHITE,1);
    }
    else if(a==2){
        write_text("Player 2 Won !!!",41,41,WHITE,1);
    }

    write_text("Press enter to exit",40,120,WHITE,0);

}

/*
    this will check if there is already a winner
*/
int checkIfWinner(){
    if(items[0]==98){
        printWinner(1);
        return 1;
    }
    else if(items[8]==98){
        printWinner(2);
        return 1;
    }
    return 0;
}

/*
    this will display the quit screen
*/
int quit_screen(){
    char p;
    drawRectangle(0,0,320,220, BLACK);
    write_text("Are you sure you want to quit?",10,41,WHITE,1);
    write_text("[y] Yes",10,160,WHITE,0);
    write_text("[n] No",150,160,WHITE,0);

    p = getchar();

    if(p==yes)
        return 1;
    else
        return 0;
}

/*
    to check if a player can have a turn
*/
int checkIfNotEmpty(int a){
    int b = a + 7;
    for(; a<b; a++){
        if(items[a]>0) return 1;
    }
    return 0;
}

/*
    this will simulate the alternating turn of each player
*/
int startGame(){
    //int currentx = 49;
    int i=0;
    char pressed;
    //getNames();
    drawRectangle(0,0,320,220, BLACK);
    drawBoard();
    drawGameNumber();

    while(1){
        if(checkIfWinner()==1){
            do{
                pressed = getchar();
            }while(pressed!=enter);
            return 1;
        }
        if(checkIfFinish()==-1){
            //prepare for the next round
            game_number ++;
            i=1;
            while(items[0]>=7 && i<8 && items[i]!=-1){
                items[i] = 7;
                items[0] = items[0] - 7;
                i++;
            }
            while(i<8){
                if(i==8) break;
                items[i]=-1;
                i++;
            }
            i=9;
            while(items[8]>=7 && i<16 && items[i]!=-1){
                items[i] = 7;
                items[8] = items[8] - 7;
                i++;
            }

            for(; i<16; i++){
                items[i]=-1;
            }

            drawBoard();
            drawGameNumber();
        }
        //player1 turns
        printTurn(1);
        drawInvertedTriangle((player1_current-1)*35+49, 30, 10, 20, WHITE);

        while(1){
            if (checkIfNotEmpty(1)==0) {
                drawRectangle((player1_current-1)*35+49, 30, 10, 8, BLACK);
                break;
            }
            pressed = getchar();
            if(pressed==left_key && player1_current!=1){
                drawRectangle((player1_current-1)*35+49, 30, 10, 8, BLACK);
                player1_current-=1;
                drawInvertedTriangle((player1_current-1)*35+49, 30, 10, 20, WHITE);
            }
            else if(pressed==right_key && player1_current!=7){
                drawRectangle((player1_current-1)*35+49, 30, 10, 8, BLACK);
                player1_current+=1;
                drawInvertedTriangle((player1_current-1)*35+49, 30, 10, 20, WHITE);
            }
            if(pressed==enter){
                if (moveItems(1)!=1){
                    drawRectangle(49, 30, 35*7, 8, BLACK);
                    break;
                }
            }
            if(pressed==quit){
                if(quit_screen()==1)
                    return 1;
                else {
                    drawRectangle(0,0,320,220, BLACK);
                    drawBoard();
                    drawGameNumber();
                    printTurn(1);
                    drawInvertedTriangle((player1_current-1)*35+49, 30, 10, 20, WHITE);
                }
            }
        }
        //player2 turns
        printTurn(2);
        drawInvertedTriangle((player2_current*-1+15)*35+49, 90, 10, 20, WHITE);
        while(1){
            if (checkIfNotEmpty(9)==0) {
                drawRectangle((player2_current*-1+15)*35+49, 90, 10, 8, BLACK);
                break;
            }
            pressed = getchar();
            if(pressed==left_key && player2_current!=15){
                drawRectangle((player2_current*-1+15)*35+49, 90, 10, 8, BLACK);
                player2_current+=1;
                drawInvertedTriangle((player2_current*-1+15)*35+49, 90, 10, 20, WHITE);
            }
            else if(pressed==right_key && player2_current!=9){
                drawRectangle((player2_current*-1+15)*35+49, 90, 10, 8, BLACK);
                player2_current-=1;
                drawInvertedTriangle((player2_current*-1+15)*35+49, 90, 10, 20, WHITE);
            }
            if(pressed==enter){
                if (moveItems(2)!=1){
                    drawRectangle(49, 90, 35*7, 8, BLACK);
                    break;
                }
            }
            if(pressed==quit){
                if(quit_screen()==1)
                    return 1;
                else {
                    drawRectangle(0,0,320,220, BLACK);
                    drawBoard();
                    drawGameNumber();
                    printTurn(2);
                    drawInvertedTriangle((player2_current*-1+15)*35+49, 90, 10, 20, WHITE);
                }
            }
        }
    }

    return 1;
}

void reinitializeVariables(){
    int a;
    player1_current = 1;
    player2_current = 15;
    game_number = 1;

    for(a=0; a<16; a++){
        if(a==0 || a==8)
            items[a]=0;
        else
            items[a]=7;
    }

}


/*
    Displays MECHANICS page given the page number
*/
char mechanics(char line[][35], int pageNo){
	int i,a;
	int linebreak = 20, skip;
	char pressed;

	drawRectangle(0,0,320,220, BLACK);
	if(pageNo==1){
		write_text(line[3], 31, 21, WHITE, 0);
		for(i=4; i<=7; i++){
			write_text(line[i], 30, 20+linebreak, WHITE, 0);
			linebreak+=20;
		}
	} else {
		write_text(line[8], 31, 21, WHITE, 0);
		for(a=1; a<=6; a++){
			skip=a*6;
			if(pageNo==a+1){
				for(i=3+skip; i<=8+skip; i++){
					write_text(line[i], 30, 20+linebreak, WHITE, 0);
					linebreak+=20;
				}
			}
		}
	}

	write_text(line[2], 30, 180, WHITE, 0); //main menu
	if(pageNo!=1) write_text(line[0], 155, 180, WHITE, 0); //back
	if(pageNo!=7) write_text(line[1], 230, 180, WHITE, 0); //next

	pressed = (char)getch();
	return pressed;
}

/*
    Houses an array of strings named line;
    allows navigation of user between pages using
    left_key and right_key or back to main menu
*/
void instruction(){
	char line[45][35];
	char pressed;
	int pageNo;

	strcpy(line[0], "[a]back");
	strcpy(line[1], "[d]next");
	strcpy(line[2], "[m]main menu");

	/*page 1*/
	strcpy(line[3], "NAVIGATION");
	//strcpy(line[4], "Press [w] to go up");
	//strcpy(line[5], "Press [s] to go down");
	strcpy(line[6], "Press [a] to go left");
	strcpy(line[7], "Press [d] to go right");

	/*page 2*/
	strcpy(line[8], "MECHANICS");
	strcpy(line[9], "Each player gets seven houses");
	strcpy(line[10], "and a head. The game moves in");
	strcpy(line[11], "a counter-clockwise direction.");
	strcpy(line[12], "Start with one of your houses.");
	strcpy(line[13], "Get the points. Log a point to");  //KAYA ATA PITONG LINES??????????
	strcpy(line[14], "each house you pass, until the");

	/*page 3*/
	strcpy(line[15], "last point. If it lands on:");
	strcpy(line[16],"(a) either player's unempty");
	strcpy(line[17], "house, get points of that");
	strcpy(line[18], "house, keep playing;");
	strcpy(line[19], "(b) your head, select again");
	strcpy(line[20], "from your houses, keep playing;");

	/*page 4*/
	strcpy(line[21], "(c) your empty house, get all");
	strcpy(line[22], "points from the adjacent house");
	strcpy(line[23], "[opponent's] add these points");
	strcpy(line[24], "to your head. Your turn ends;");
	strcpy(line[25], "(d) your opponent's empty");
	strcpy(line[26], "house, your turn ends. A game");

	/*page 5*/
	strcpy(line[27], "goes for several rounds. A round");
	strcpy(line[28], "ends if all points are in");
	strcpy(line[29], "respective heads. In succeeding");
	strcpy(line[30], "rounds, gained points are");
	strcpy(line[31], "redistributed to respective");
	strcpy(line[32], "houses. If remaining points do");

	/*page 6*/
	strcpy(line[33], "not suffice for a house, that");
	strcpy(line[34], "house becomes `burnt`. Burnt");
	strcpy(line[35], "houses are unusable, and cannot");
	strcpy(line[36], "be filled in later rounds. Game");
	strcpy(line[37], "ends when one player burns all");
	strcpy(line[38], "his houses.");

	/*page 7*/
	strcpy(line[39], "Goal: obtain all 98 points,");
	strcpy(line[40], "and burn all opponent's");
	strcpy(line[41], "houses!!!!!!!!!!!!");


	pageNo=1;
	pressed = mechanics(line, pageNo);
	if(pressed==right_key){ pageNo+=1; }

	do{
		switch(pressed){
			case right_key : 	pressed = mechanics(line, pageNo);
							if(pressed == right_key && pageNo<7){ pageNo+=1; }
							else if (pressed == left_key){ pageNo-=1; }
							break;

			case left_key : pressed = mechanics(line, pageNo);
                            if(pressed == right_key){ pageNo+=1; }
							else if (pressed == left_key && pageNo>1){ pageNo-=1;}
							break;
		}
	} while (pressed != main_menu);


}

main(){
    char pressed;
    set_graphics(VGA_320X200X256);

    do{
        drawMenu();
        reinitializeVariables();
        pressed = (char)getch();
        if(pressed==enter){
            startGame();
        }
        else if(pressed=='i'){
            instruction();
        }
    }while(pressed!=quit);
    //Return ICS-OS graphics before exiting
	set_graphics(VGA_TEXT80X25X16);
	clrscr();
	exit(0);
}
