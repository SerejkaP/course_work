#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>

#define NUMBER_BOYS 12 //Количество юношей
#define NUMBER_GIRLS 15 //Количество девушек

pthread_t TIMEThread; //Нить вспомогательного таймера
pthread_mutex_t DMutex; //Мьютекс для отрисовки

int girls[NUMBER_GIRLS]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};//Массив для проверки занятости девушки
const int dance_positions[15][2]={{18,8},{18,10},{18,12},{22,8},{22,10},{22,12},{26,8},{26,10},{26,12},{30,8},{30,10},{30,12},{34,8},{34,10},{34,12}}; //Координаты мест для танца
const int boys_positions[NUMBER_BOYS]={3,6,9,12,15,18,21,24,27,30,33,36}; //Начальные координаты по X для юношей
const int girls_positions[NUMBER_GIRLS]={6,8,10,12,14,16,18,20,22,24,26,28,30,32,34}; //Начальные координаты по X для девушек
char room[18][52]={
	"________________DANCE________FLOOR_________________",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|                                                  |",
	"|__________________________________________________|",
};//Модель комнаты

int free_girl=0; //переменная для установки и проверки номера свободной девушки
char A = 'A'; //для присваивания имён юношам
char a = 'a'; //для присваивания имён девушкам
int times;
bool key; //Выключатель для проверки времени цикла

struct girls{ //Структура для девушки
	char name;
	int number;
	int id;
	int coord[2];
	bool key;
	pthread_mutex_t WaitMutex;
	pthread_mutex_t FreeMutex;
	pthread_t GIRLThreads;
};

struct boys{ //Структура для юноши
	char name;
	int number;
	int coord[2];
	bool key;
	pthread_mutex_t WaitMutex;
	pthread_mutex_t DanceMutex;
	pthread_t BOYThreads;
};

struct girls girl[NUMBER_GIRLS];//Массив структур девушек
struct boys boy[NUMBER_BOYS];//Массив структур юношей

void color(int colorNumber) // Задаёт цвет букве
{
    colorNumber = colorNumber % 7 + 1;
    switch (colorNumber)
    {
    case 1:
        printf("\x1b[1;31m");
        break;
    case 2:
        printf("\x1b[1;32m");
        break;
    case 3:
        printf("\x1b[1;33m");
        break;
    case 4:
        printf("\x1b[1;34m");
        break;
    case 5:
        printf("\x1b[1;35m");
        break;
    case 6:
        printf("\x1b[1;36m");
        break;
    case 7:
        printf("\x1b[1;37m");
        break;
    }
}

void draw_person(int x, int y, char name){ //Рисует символ в цвете
		pthread_mutex_lock(&DMutex);
		color((int)name);
		printf("\033[%d;%dH%c\n",y,x,name);
		pthread_mutex_unlock(&DMutex);
}

void go_to_position(int x1, int y1, int x2, int y2, char name, int how){ //Перемещает символ
	draw_person(x1,y1, ' '); //Стирает старый символ
	int sign;//знак - в какую строну идем
	int sum=0;//вспомогательная переменная для how. определяет по какой координате идти и куда уже шли
	for (int i=how; i<=how+1;i++)
	{
		int from=0;//откуда (по х и по у)
		if (i%2==0)//если четное - то сначала по у идем
		{ 	//идти по y
			sign = (y2>=y1)?1:-1;//задаем в какую сторону
			from = (sum==0)?x1:x2;//и куда
			for (int y=y1; y!=y2; y+=sign)
			{
				if(y!=y1) draw_person(from, y-sign, ' ');//затираем предыдущую точку
				draw_person(from, y,name);//пишем в точке имя
				usleep(30000);//ждем немного
			}
			if (y1!=y2) draw_person(from, y2-sign, ' ');//затираем предпоследнюю точку
		}
		else//иначе - по х
		{	//идти по х
			sign = (x2>=x1)?1:-1;//Проверка направления по X
			from = (sum==0)?y1:y2;//Проверка направления по Y
			for (int x = x1; x!=x2; x+=sign)
			{
				if (x!=x1) draw_person(x-sign, from, ' '); //Стирает старый символ
				draw_person(x, from,name); //Рисует символ в новой точке
				usleep(30000);//Задержка для плавности перемещения
			}
			if (x1!=x2) draw_person(x2-sign, from, ' ');//Стирает старый символ
		}
		sum++;
	}
	draw_person(x2,y2,name);//Рисует символ в необходимой точке
}

void boys_start_position(){ //Определение начальных позиций для юношей
	for(int i=0;i<NUMBER_BOYS;i++){
		boy[i].coord[0]=boys_positions[i];//Передача координат в структуру
		boy[i].coord[1]=3;
		draw_person(boy[i].coord[0],boy[i].coord[1],boy[i].name);//Отрисовка на начальной позиции	
	}
}

void girls_start_position(){//Определение начальных позиций для девушек
	for(int i=0;i<NUMBER_GIRLS;i++){
		girl[i].coord[0]=girls_positions[i];//Передача координат в структуру
		girl[i].coord[1]=18;
		draw_person(girl[i].coord[0],girl[i].coord[1],girl[i].name); //Отрисовка на начальной позиции
	}
}

int take_positionX(int number){//Определение координат места для танца по X
	return dance_positions[number][0];
}

int take_positionY(int number){//Определение координат места для танца по Y
	return dance_positions[number][1];
}

void abstr_girls(void* arg){//Абстрактная модель девушки
	int first_coords[2];
	int second_coords[2];
	pthread_mutex_lock(&girl[(int)arg].WaitMutex);
	girl[(int)arg].name=a+(int)arg;
	girl[(int)arg].number=(int)arg;
	girls_start_position();//Помещение девушек на стартовую позицию
	usleep(8000000);
	while(1){
		if (girls[(int)arg]==1){//Если девушка занята на танец
			pthread_mutex_lock(&boy[girl[(int)arg].id].WaitMutex);
			pthread_mutex_lock(&girl[(int)arg].WaitMutex);
		}
	}
}

void abstr_boys(void* arg){ // Абстрактная модель юноши
	int first_coords[2];
	int second_coords[2];
	int xy1[2];
	int xy2[2];
	int number;
	int time=5;//Количество циклов
	boy[(int)arg].name=A+(int)arg;
	boy[(int)arg].key=false;
	pthread_mutex_lock(&boy[(int)arg].WaitMutex);
	pthread_mutex_lock(&boy[(int)arg].DanceMutex);
	boys_start_position();//Помощенеи юношей на стартовую позицию
	while(time>0){//Начало цикла танца
		usleep(8000000);
		number=rand()%NUMBER_GIRLS;//Выбирает девушку
		boy[(int)arg].number=number;
		while(girls[number]!=0){//В случае, если занята, ищет новую
			number=rand()%NUMBER_GIRLS;
			boy[(int)arg].number=number;
		}
		girl[number].id=(int)arg;//Выбранная девушка запоминает юношу
		pthread_mutex_lock(&girl[number].FreeMutex);
		girls[number]=1;//Устанавливает 1, т.е. девушка занята на танец
		first_coords[0]=boy[(int)arg].coord[0];
		first_coords[1]=boy[(int)arg].coord[1];
		second_coords[0]=girl[boy[(int)arg].number].coord[0];
		second_coords[1]=girl[boy[(int)arg].number].coord[1]-1;
		go_to_position(first_coords[0],first_coords[1],second_coords[0],second_coords[1],boy[(int)arg].name,1);//Идёт к девушке
		boy[(int)arg].key=true;//Помечает, что юноша пошёл танцевать
		first_coords[0]=second_coords[0];
		first_coords[1]=second_coords[1];
		second_coords[0]=take_positionX(boy[(int)arg].number)+1;
		second_coords[1]=take_positionY(boy[(int)arg].number);	go_to_position(first_coords[0],first_coords[1],second_coords[0],second_coords[1],boy[(int)arg].name,0);//Идёт танцевать
		xy1[0]=girl[number].coord[0];//Перемещение девушки на танец
		xy1[1]=girl[number].coord[1];
		xy2[0]=take_positionX(girl[number].number);
		xy2[1]=take_positionY(girl[number].number);
		go_to_position(xy1[0],xy1[1],xy2[0],xy2[1],girl[number].name,0);
		
		draw_person(second_coords[0],second_coords[1],boy[(int)arg].name);
		draw_person(xy2[0],xy2[1],girl[number].name);
		
		pthread_mutex_lock(&boy[(int)arg].DanceMutex);
		
		draw_person(second_coords[0],second_coords[1],boy[(int)arg].name);
		draw_person(xy2[0],xy2[1],girl[number].name);
		
		while(key==false){ //Пока вспомогательный "таймер" выключил ключ идёт танец
			draw_person(second_coords[0],second_coords[1],boy[(int)arg].name); //Отрисовка на позициях, в случае затирания символа
			draw_person(xy2[0],xy2[1],girl[number].name);
			usleep(10000);
		}
		boy[(int)arg].key=false;//Помечает, что танец закончился
		first_coords[0]=second_coords[0];
		first_coords[1]=second_coords[1];
		second_coords[0]=boy[(int)arg].coord[0];
		second_coords[1]=boy[(int)arg].coord[1];
		go_to_position(first_coords[0],first_coords[1],second_coords[0],second_coords[1],boy[(int)arg].name,1);//Идёт на своё место
		go_to_position(xy2[0],xy2[1],xy1[0],xy1[1],girl[number].name,1); //Девушка идёт на своё место
		
		pthread_mutex_unlock(&girl[number].WaitMutex);//Освобождение девушки
		pthread_mutex_unlock(&girl[number].FreeMutex);
		
		first_coords[0]=second_coords[0];
		first_coords[1]=second_coords[1];
		second_coords[0]=boy[(int)arg].coord[0];
		second_coords[1]=boy[(int)arg].coord[1];
		go_to_position(first_coords[0],first_coords[1],second_coords[0],second_coords[1],boy[(int)arg].name,1);//Идёт на своё место
		girls[boy[(int)arg].number]=0;//Устанавливает 0, т.е. девушка свободна для танца
		usleep(3000000);
		time--;//Завершает итерацию цикла
	}
}

void *timer_function(void* arg){//Вспомогательный поток, отвечающий за время танца
	while(times>0){//Повторяет 5 раз
	printf("\033[20;2H\033[1;34mТанцев:   ");
	printf("\033[20;2H\033[1;34mТанцев: %d",times);
	if(times==1) printf("\033[20;2H\033[1;34mТанцев: %d, последний",times);
	usleep(8000000);
	usleep(7000000);//Поправка времени на передвижение
	key=false;//Выключение ключа для начала времени танца
	for(int i=0;i<NUMBER_BOYS;i++)
		if(boy[i].key==true) pthread_mutex_unlock(&boy[i].DanceMutex);//Если пошёл танцевать, то разблокирует мьютекс танца
	usleep(20000000);//Танец 20 секунд
	key=true;//Включение ключа
	if(times==1){
		printf("\033[20;2H\033[1;34m                     ");
		printf("\033[20;2H\033[1;34mКонец...");
	}
	times--;//Завершение итерации цикла
	usleep(3000000);
	}
}

void main(){
	printf("\033[H\033[2J"); //Очистка экрана
	printf("\n");
	int rc;
	times=5; //Количество циклов
	for (int i=0;i<18;i++){ //Создание рисунка комнаты
		for (int j=0;j<52;j++){
			printf("\x1b[31m%c", room[i][j]);
		}
	printf("\n");
	}
	printf("\033[20;2H\033[1;34mТанцев: ");
	rc=pthread_mutex_init(&DMutex, NULL);
	for(int i=0;i<NUMBER_GIRLS;i++){ //Создание нитей для девушек
		rc=pthread_create(&girl[i].GIRLThreads, NULL, (void *)abstr_girls, (void*)i);
		if(rc!=0) printf("ERROR");
	}
	for(int i=0;i<NUMBER_BOYS;i++){ //Создание нитей для юношей
		rc=pthread_create(&boy[i].BOYThreads, NULL, (void *)abstr_boys, (void*)i);
		if(rc!=0) printf("ERROR");
	}
	rc=pthread_create(&TIMEThread, NULL, (void *)timer_function, NULL);
	if(rc!=0) printf("ERROR");
	getchar();
}

