#include "Render.h"




#include <windows.h>

#include <GL\gl.h>
#include <GL\glu.h>
#include "GL\glext.h"

#include "MyOGL.h"
#include "MyVector3d.h"
#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "MyShaders.h"

#include "ObjLoader.h"
#include "GUItextRectangle.h"

#include "Texture.h"

GuiTextRectangle rec;

bool textureMode = true;
bool lightMode = true;


//небольшой дефайн для упрощения кода
#define POP glPopMatrix()
#define PUSH glPushMatrix()



ObjFile *model;

Texture texture1;
Texture sTex;
Texture rTex;
Texture tBox;

Shader s[10];  //массивчик для десяти шейдеров
Shader frac;
Shader cassini;

double t_max = 0;
double delta_time = 0.1;
double delta_time1 = 0.1;


//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	virtual void SetUpCamera()
	{

		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//класс недоделан!
class WASDcamera :public CustomCamera
{
public:
		
	float camSpeed;

	WASDcamera()
	{
		camSpeed = 0.4;
		pos.setCoords(5, 5, 5);
		lookPoint.setCoords(0, 0, 0);
		normal.setCoords(0, 0, 1);
	}

	virtual void SetUpCamera()
	{

		if (OpenGL::isKeyPressed('W'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*camSpeed;
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}
		if (OpenGL::isKeyPressed('S'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*(-camSpeed);
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}

		LookAt();
	}

} WASDcam;


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);
		
		glDisable(GL_DEPTH_TEST);
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
				glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}
		/*
		if (f1)
			glEnable(GL_LIGHTING);
		if (f2)
			glEnable(GL_TEXTURE_2D);
		if (f3)
			glEnable(GL_DEPTH_TEST);
			*/
	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света



//старые координаты мыши
int mouseX = 0, mouseY = 0;




float offsetX = 0, offsetY = 0;
float zoom=1;
float Time = 0;
int tick_o = 0;
int tick_n = 0;

//обработчик движения мыши
void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}


	if (OpenGL::isKeyPressed(VK_LBUTTON))
	{
		offsetX -= 1.0*dx/ogl->getWidth()/zoom;
		offsetY += 1.0*dy/ogl->getHeight()/zoom;
	}


	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y,60,ogl->aspect);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}


	
}

//обработчик вращения колеса  мыши
void mouseWheelEvent(OpenGL *ogl, int delta)
{


	float _tmpZ = delta*0.003;
	if (ogl->isKeyPressed('Z'))
		_tmpZ *= 10;
	zoom += 0.2*zoom*_tmpZ;


	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;
}

bool flag = false;
bool shar = false;
//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}	   

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}

	if (key == 'S')
	{
		frac.LoadShaderFromFile();
		frac.Compile();

		s[0].LoadShaderFromFile();
		s[0].Compile();

		cassini.LoadShaderFromFile();
		cassini.Compile();
	}

	if (OpenGL::isKeyPressed('D'))
	{
		flag = !flag;
	}

	if (OpenGL::isKeyPressed('F'))
	{
		flag = false;
	}

	if (OpenGL::isKeyPressed('Z'))
	{
		shar = !shar;
	}

	if (key == 'Q')
		Time = 0;
}

void keyUpEvent(OpenGL *ogl, int key)
{

}


void DrawQuad()
{
	double A[] = { 0,0 };
	double B[] = { 1,0 };
	double C[] = { 1,1 };
	double D[] = { 0,1 };
	glBegin(GL_QUADS);
	glColor3d(.5, 0, 0);
	glNormal3d(0, 0, 1);
	glTexCoord2d(0, 0);
	glVertex2dv(A);
	glTexCoord2d(1, 0);
	glVertex2dv(B);
	glTexCoord2d(1, 1);
	glVertex2dv(C);
	glTexCoord2d(0, 1);
	glVertex2dv(D);
	glEnd();
}



ObjFile draco;

Texture td;


ObjFile draco2;

Texture td2;//menati


ObjFile odwe,odwe1,odwe2,odwe3;

Texture odw;


ObjFile zamok;

Texture zz;

ObjFile zamokd;

Texture zzd;


ObjFile st,st1,st2;

Texture s3;
Texture s1;
Texture s2;

ObjFile st13, st11, st21;

Texture s5;
Texture s6;
Texture s7;

//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{

	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	
	


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	//ogl->mainCamera = &WASDcam;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	/*
	//texture1.loadTextureFromFile("textures\\texture.bmp");   загрузка текстуры из файла
	*/


	frac.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	frac.FshaderFileName = "shaders\\frac.frag"; //имя файла фрагментного шейдера
	frac.LoadShaderFromFile(); //загружаем шейдеры из файла
	frac.Compile(); //компилируем

	cassini.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	cassini.FshaderFileName = "shaders\\cassini.frag"; //имя файла фрагментного шейдера
	cassini.LoadShaderFromFile(); //загружаем шейдеры из файла
	cassini.Compile(); //компилируем
	

	s[0].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[0].FshaderFileName = "shaders\\light.frag"; //имя файла фрагментного шейдера
	s[0].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[0].Compile(); //компилируем

	s[1].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[1].FshaderFileName = "shaders\\textureShader.frag"; //имя файла фрагментного шейдера
	s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[1].Compile(); //компилируем

	

	 //так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	 // создающимися во время компиляции, я переименовал модели в *.obj_m

	loadModel("models\\draco.obj_m", &draco);//dragon
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\draco.obj_m", &draco);
	td.loadTextureFromFile("textures//td.bmp");
	td.bindTexture();

	loadModel("models\\draco2.obj_m", &draco2);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\draco2.obj_m", &draco2);
	td2.loadTextureFromFile("textures//draco2.bmp");
	td2.bindTexture();


	loadModel("models\\odw.obj_m", &odwe);//oblaca
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\odw.obj_m", &odwe);
	odw.loadTextureFromFile("textures//odw.bmp");
	odw.bindTexture();


	loadModel("models\\odw.obj_m", &odwe1);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\odw.obj_m", &odwe1);
	odw.loadTextureFromFile("textures//odw.bmp");
	odw.bindTexture();

	loadModel("models\\odw.obj_m", &odwe2);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\odw.obj_m", &odwe2);
	odw.loadTextureFromFile("textures//odw.bmp");
	odw.bindTexture();

	loadModel("models\\odw.obj_m", &odwe3);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\odw.obj_m", &odwe3);
	odw.loadTextureFromFile("textures//odw.bmp");
	odw.bindTexture();

	loadModel("models\\zamok.obj_m", &zamok);///zamok
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\zamok.obj_m", &zamok);
	zz.loadTextureFromFile("textures//zan1.bmp");
	zz.bindTexture();

	loadModel("models\\zamok1.obj_m", &zamokd);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\zamok1.obj_m", &zamokd);
	zzd.loadTextureFromFile("textures//zamb.bmp");
	zzd.bindTexture();


	loadModel("models\\st.obj_m", &st);//star
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\st.obj_m", &st);
	s1.loadTextureFromFile("textures//s.bmp");
	s1.bindTexture();

	loadModel("models\\st.obj_m", &st1);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\st.obj_m", &st1);
	s2.loadTextureFromFile("textures//s2.bmp");
	s2.bindTexture();

	loadModel("models\\st.obj_m", &st2);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\st.obj_m", &st2);
	s3.loadTextureFromFile("textures//s4.bmp");
	s3.bindTexture();

	loadModel("models\\st1.obj_m", &st13);//star2
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\st1.obj_m", &st13);
	s5.loadTextureFromFile("textures//s5.bmp");
	s5.bindTexture();

	loadModel("models\\st1.obj_m", &st11);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\st1.obj_m", &st11);
	s6.loadTextureFromFile("textures//s6.bmp");
	s6.bindTexture();

	loadModel("models\\st1.obj_m", &st21);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\st1.obj_m", &st21);
	s7.loadTextureFromFile("textures//s7.bmp");
	s7.bindTexture();




	tick_n = GetTickCount();
	tick_o = tick_n;

	rec.setSize(400, 100);
	rec.setPosition(10, ogl->getHeight() - 100-10);
	rec.setText("T - вкл/выкл текстур\nD - Темная тема\nF - Светлая тема\nZ - Звездопад",0,0,0);

	
}
//#pragma bize

#pragma region MyDragon



double f(double p1, double p2, double p3, double p4, double t)
{
	return p1 * (1 - t) * (1 - t) * (1 - t) + 3 * p2 * t * (1 - t) * (1 - t) + 3 * p3 * t * t * (1 - t) + p4 * t * t * t; //посчитанная формула
}

void bize(double P1[3], double P2[3], double P3[3], double P4[3])
{
	glDisable(GL_LIGHTING);
	glColor3d(1, 0.5, 0);
	glLineWidth(3); //ширина линии
	glBegin(GL_LINE_STRIP);
	for (double t = 0; t <= 1.0001; t += 0.01)
	{
		double P[3];
		P[0] = f(P1[0], P2[0], P3[0], P4[0], t);
		P[1] = f(P1[1], P2[1], P3[1], P4[1], t);
		P[2] = f(P1[2], P2[2], P3[2], P4[2], t);
		glVertex3dv(P); //Рисуем точку P
	}
	glEnd();
	glColor3d(0, 0, 1);
	glLineWidth(1); //возвращаем ширину линии = 1

	glBegin(GL_LINES);
	glVertex3dv(P1);
	glVertex3dv(P2);
	glVertex3dv(P2);
	glVertex3dv(P3);
	glVertex3dv(P3);
	glVertex3dv(P4);
	glEnd();
	glEnable(GL_LIGHTING);
}

Vector3 bizeWithoutDraw(double P1[3], double P2[3], double P3[3], double P4[3], double t)
{
	Vector3 Vec;
	Vec.setCoords(f(P1[0], P2[0], P3[0], P4[0], t), f(P1[1], P2[1], P3[1], P4[1], t), f(P1[2], P2[2], P3[2], P4[2], t));
	return Vec;
}

void Bese2(double P1[3], double P2[3], double P3[3], double P4[3], double delta_time)
{
	static double t_max = 0;
	static bool flagReverse = false;

	if (!flagReverse)
	{
		t_max += delta_time / 20; //t_max становится = 1 за 5 секунд
		if (t_max > 1)
		{
			t_max = 1; //после обнуляется
			flagReverse = !flagReverse;
		}
	}
	else
	{
		t_max -= delta_time / 20; //t_max становится = 1 за 5 секунд
		if (t_max < 0)
		{
			t_max = 0; //после обнуляется
			flagReverse = !flagReverse;
		}
	}

	//bize(P1, P2, P3, P4);//чтобы не рисовало кривую удали строчку

	Vector3 P_old = bizeWithoutDraw(P1, P2, P3, P4, !flagReverse ? t_max - delta_time : t_max + delta_time);
	Vector3 P = bizeWithoutDraw(P1, P2, P3, P4, t_max);
	Vector3 VecP_P_old = (P - P_old).normolize();

	Vector3 rotateX(VecP_P_old.X(), VecP_P_old.Y(), 0);
	rotateX = rotateX.normolize();

	Vector3 VecPrX = Vector3(1, 0, 0).vectProisvedenie(rotateX);
	double CosX = Vector3(1, 0, 0).ScalarProizv(rotateX);
	double SinAngleZ = VecPrX.Z() / abs(VecPrX.Z());
	double AngleOZ = acos(CosX) * 180 / PI * SinAngleZ;

	double AngleOY = acos(VecP_P_old.Z()) * 180 / PI - 90;

	double A[] = { -0.5,-0.5,-0.5 };
	glPushMatrix();
	glTranslated(P.X(), P.Y(), P.Z());

	glRotated(AngleOZ, 0, 0, 1);
	glRotated(AngleOY, 0, 1, 0);
	

	glRotated(-90, 0, 0, 1);
	glScaled(0.5, 0.5, 0.5);
	td.bindTexture();
	draco.DrawObj();
	
	glPopMatrix();

	glColor3d(0, 0, 0);

}

void Bese1(double P1[3], double P2[3], double P3[3], double P4[3], double delta_time)
{
	static double t_max = 0;
	static bool flagReverse = false;

	if (!flagReverse)
	{
		t_max += delta_time / 20; //t_max становится = 1 за 5 секунд
		if (t_max > 1)
		{
			t_max = 1; //после обнуляется
			flagReverse = !flagReverse;
		}
	}
	else
	{
		t_max -= delta_time / 20; //t_max становится = 1 за 5 секунд
		if (t_max < 0)
		{
			t_max = 0; //после обнуляется
			flagReverse = !flagReverse;
		}
	}

	//bize(P1, P2, P3, P4);//чтобы не рисовало кривую удали строчку

	Vector3 P_old = bizeWithoutDraw(P1, P2, P3, P4, !flagReverse ? t_max - delta_time : t_max + delta_time);
	Vector3 P = bizeWithoutDraw(P1, P2, P3, P4, t_max);
	Vector3 VecP_P_old = (P - P_old).normolize();

	Vector3 rotateX(VecP_P_old.X(), VecP_P_old.Y(), 0);
	rotateX = rotateX.normolize();

	Vector3 VecPrX = Vector3(1, 0, 0).vectProisvedenie(rotateX);
	double CosX = Vector3(1, 0, 0).ScalarProizv(rotateX);
	double SinAngleZ = VecPrX.Z() / abs(VecPrX.Z());
	double AngleOZ = acos(CosX) * 180 / PI * SinAngleZ;

	double AngleOY = acos(VecP_P_old.Z()) * 180 / PI - 90;

	double A[] = { -0.5,-0.5,-0.5 };
	glPushMatrix();
	glTranslated(P.X(), P.Y(), P.Z());

	glRotated(AngleOZ, 0, 0, 1);
	glRotated(AngleOY, 0, 1, 0);
	
	//glScaled(0.1, 0.1, 0.1);
	td2.bindTexture();
	draco2.DrawObj();
	
	glPopMatrix();

	

}

void Style2() {

	glPopMatrix();
	double P11[] = { 5,-20,10 }; //Наши точки
	double P12[] = { -4,6,15 };
	double P13[] = { 5,5,7 };
	double P14[] = { -10,15,8 };
	Bese1(P11, P12, P13, P14, delta_time1);

	glPopMatrix();
	double P1[] = { 15,-20,10 }; //Наши точки
	double P2[] = { 4,-16,5 };
	double P3[] = { 5,20,10 };
	double P4[] = { -20,10,15 };
	Bese2(P1, P2, P3, P4, delta_time);

	//glRotated(-90, -90, 0, 1);
	
	glPushMatrix();
	glScaled(0.1, 0.1, 0.1);
	//указываем текстуру, которую будем использовать
	zzd.bindTexture();
	//указываем модель которую хотим нарисовать
	zamokd.DrawObj();
	glPopMatrix();

	
}


void Style1() {

	glPopMatrix();
	double P1[] = { 10,-20,10 }; //Наши точки
	double P2[] = { 4,-16,5 };
	double P3[] = { 5,15,10 };
	double P4[] = { -20,10,10 };
	Bese2(P1, P2, P3, P4, delta_time);


	glPushMatrix();
	glScaled(0.1, 0.1, 0.1);
	//указываем текстуру, которую будем использовать
	zz.bindTexture();
	//указываем модель которую хотим нарисовать
	zamok.DrawObj();
	glPopMatrix();

}

double x = 0;
double y = 0;
double h = 10;
bool Jamp = true;

void Cloudw() {

	if (h <= 10, x <= 0, y <=0)
	{
		h = 10;
		x = 0;
	    y = 0;
		Jamp = true;
	}
	if (h >= 15, x >= 5, y >= 5)
	{
		h = 15;
		x = 5;
		y = 5;
		Jamp = false;
	}
	if (Jamp == true)
	{
		h += 0.01;
		x += 0.1;
		y += 0.1;
		glPushMatrix();
		glTranslated(x, y, h);
		//указываем текстуру, которую будем использовать
		odw.bindTexture();
		//указываем модель которую хотим нарисовать
		odwe.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glTranslated(x, y, h);
		//указываем текстуру, которую будем использовать
		glRotated(-90, 0, 0, 1);
		odw.bindTexture();
		//указываем модель которую хотим нарисовать
		odwe1.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glTranslated(x, y, h);
		//указываем текстуру, которую будем использовать
		glRotated(90, 0, 0, 1);
		odw.bindTexture();
		//указываем модель которую хотим нарисовать
		odwe2.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glTranslated(x, y, h);
		//указываем текстуру, которую будем использовать
		glRotated(180, 0, 0, 1);
		odw.bindTexture();
		//указываем модель которую хотим нарисовать
		odwe3.DrawObj();
		glPopMatrix();
	}
	else
	{
		h -= 0.01;
		x -= 0.1;
		y -= 0.1;
		glPushMatrix();
		glTranslated(x, y, h);
		//указываем текстуру, которую будем использовать
		odw.bindTexture();
		//указываем модель которую хотим нарисовать
		odwe.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glTranslated(x, y, h);
		//указываем текстуру, которую будем использовать
		glRotated(-90, 0, 0, 1);
		odw.bindTexture();
		//указываем модель которую хотим нарисовать
		odwe1.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glTranslated(x, y, h);
		//указываем текстуру, которую будем использовать
		glRotated(90, 0, 0, 1);
		odw.bindTexture();
		//указываем модель которую хотим нарисовать
		odwe2.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glTranslated(x, y, h);
		//указываем текстуру, которую будем использовать
		glRotated(180, 0, 0, 1);
		odw.bindTexture();
		//указываем модель которую хотим нарисовать
		odwe3.DrawObj();
		glPopMatrix();
	}


}

double z = 15;
bool Jump = false;

void Shar1() {
	if (z <= 0)
	{
		z = 0;
		Jump = true;
	}
	if (z >= 15)
	{
		z = 15;
		Jump = false;
	}
	if (Jump == true)
	{
		z += 0.1;
		glPushMatrix();
		glTranslated(20, -15, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(60, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s1.bindTexture();
		//указываем модель которую хотим нарисовать
		st.DrawObj();
		glPopMatrix();
		
		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-20, 13, z);
		//указываем текстуру, которую будем использовать
		glRotated(-40, 90, 0, 1);
		s1.bindTexture();
		//указываем модель которую хотим нарисовать
		st.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(20, 18, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(0, 70, 0, 1);
		//указываем текстуру, которую будем использовать
		s2.bindTexture();
		//указываем модель которую хотим нарисовать
		st1.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(20, -10, z);
		//указываем текстуру, которую будем использовать
		glRotated(-20, 90, 0, 1);
		s2.bindTexture();
		//указываем модель которую хотим нарисовать
		st1.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(18, 4, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(0, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s3.bindTexture();
		//указываем модель которую хотим нарисовать
		st2.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-15, 20, z);
		//указываем текстуру, которую будем использовать
		glRotated(-70, 90, 0, 1);
		s3.bindTexture();
		//указываем модель которую хотим нарисовать
		st2.DrawObj();
		glPopMatrix();





		glPushMatrix();
		glTranslated(14, 10, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(60, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s1.bindTexture();
		//указываем модель которую хотим нарисовать
		st.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(8, -19, z);
		//указываем текстуру, которую будем использовать
		glRotated(-90, 70, 0, 1);
		s1.bindTexture();
		//указываем модель которую хотим нарисовать
		st.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(10, 5, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(180, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s2.bindTexture();
		//указываем модель которую хотим нарисовать
		st1.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(2, -12, z);
		//указываем текстуру, которую будем использовать
		glRotated(-50, -90, 0, 1);
		s2.bindTexture();
		//указываем модель которую хотим нарисовать
		st1.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(-12, -15, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(-30, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s3.bindTexture();
		//указываем модель которую хотим нарисовать
		st2.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-6, 10, z);
		//указываем текстуру, которую будем использовать
		glRotated(-90, -50, 0, 1);
		s3.bindTexture();
		//указываем модель которую хотим нарисовать
		st2.DrawObj();
		glPopMatrix();
		
	}

	else {
		z -= 0.1;
		glPushMatrix();
		glTranslated(20, -15, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(60, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s1.bindTexture();
		//указываем модель которую хотим нарисовать
		st.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-20, 13, z);
		//указываем текстуру, которую будем использовать
		glRotated(-40, 90, 0, 1);
		s1.bindTexture();
		//указываем модель которую хотим нарисовать
		st.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(20, 18, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(0, 70, 0, 1);
		//указываем текстуру, которую будем использовать
		s2.bindTexture();
		//указываем модель которую хотим нарисовать
		st1.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(20, -10, z);
		//указываем текстуру, которую будем использовать
		glRotated(-20, 90, 0, 1);
		s2.bindTexture();
		//указываем модель которую хотим нарисовать
		st1.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(18, 4, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(0, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s3.bindTexture();
		//указываем модель которую хотим нарисовать
		st2.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-15, 20, z);
		//указываем текстуру, которую будем использовать
		glRotated(-70, 90, 0, 1);
		s3.bindTexture();
		//указываем модель которую хотим нарисовать
		st2.DrawObj();
		glPopMatrix();





		glPushMatrix();
		glTranslated(14, 10, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(60, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s1.bindTexture();
		//указываем модель которую хотим нарисовать
		st.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(8, -19, z);
		//указываем текстуру, которую будем использовать
		glRotated(-90, 70, 0, 1);
		s1.bindTexture();
		//указываем модель которую хотим нарисовать
		st.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(10, 5, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(180, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s2.bindTexture();
		//указываем модель которую хотим нарисовать
		st1.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(2, -12, z);
		//указываем текстуру, которую будем использовать
		glRotated(-50, -90, 0, 1);
		s2.bindTexture();
		//указываем модель которую хотим нарисовать
		st1.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(-12, -15, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(-30, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s3.bindTexture();
		//указываем модель которую хотим нарисовать
		st2.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-6, 10, z);
		//указываем текстуру, которую будем использовать
		glRotated(-90, -50, 0, 1);
		s3.bindTexture();
		//указываем модель которую хотим нарисовать
		st2.DrawObj();
		glPopMatrix();
	}
	
}

void Shar2() {
	if (z <= 0)
	{
		z = 0;
		Jump = true;
	}
	if (z >= 15)
	{
		z = 15;
		Jump = false;
	}
	if (Jump == true)
	{
		z += 0.1;
		glPushMatrix();
		glTranslated(20, -15, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(60, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s5.bindTexture();
		//указываем модель которую хотим нарисовать
		st13.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-20, 13, z);
		//указываем текстуру, которую будем использовать
		glRotated(-40, 90, 0, 1);
		s5.bindTexture();
		//указываем модель которую хотим нарисовать
		st13.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(20, 18, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(0, 70, 0, 1);
		//указываем текстуру, которую будем использовать
		s6.bindTexture();
		//указываем модель которую хотим нарисовать
		st11.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(20, -10, z);
		//указываем текстуру, которую будем использовать
		glRotated(-20, 90, 0, 1);
		s6.bindTexture();
		//указываем модель которую хотим нарисовать
		st11.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(18, 4, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(0, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s7.bindTexture();
		//указываем модель которую хотим нарисовать
		st21.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-15, 20, z);
		//указываем текстуру, которую будем использовать
		glRotated(-70, 90, 0, 1);
		s7.bindTexture();
		//указываем модель которую хотим нарисовать
		st21.DrawObj();
		glPopMatrix();





		glPushMatrix();
		glTranslated(14, 10, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(60, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s5.bindTexture();
		//указываем модель которую хотим нарисовать
		st13.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(8, -19, z);
		//указываем текстуру, которую будем использовать
		glRotated(-90, 70, 0, 1);
		s5.bindTexture();
		//указываем модель которую хотим нарисовать
		st13.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(10, 5, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(180, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s6.bindTexture();
		//указываем модель которую хотим нарисовать
		st11.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(2, -12, z);
		//указываем текстуру, которую будем использовать
		glRotated(-50, -90, 0, 1);
		s6.bindTexture();
		//указываем модель которую хотим нарисовать
		st11.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(-12, -15, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(-30, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s7.bindTexture();
		//указываем модель которую хотим нарисовать
		st21.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-6, 10, z);
		//указываем текстуру, которую будем использовать
		glRotated(-90, -50, 0, 1);
		s7.bindTexture();
		//указываем модель которую хотим нарисовать
		st21.DrawObj();
		glPopMatrix();

	}

	else {
		z -= 0.1;
		glPushMatrix();
		glTranslated(20, -15, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(60, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s5.bindTexture();
		//указываем модель которую хотим нарисовать
		st13.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-20, 13, z);
		//указываем текстуру, которую будем использовать
		glRotated(-40, 90, 0, 1);
		s5.bindTexture();
		//указываем модель которую хотим нарисовать
		st13.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(20, 18, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(0, 70, 0, 1);
		//указываем текстуру, которую будем использовать
		s6.bindTexture();
		//указываем модель которую хотим нарисовать
		st11.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(20, -10, z);
		//указываем текстуру, которую будем использовать
		glRotated(-20, 90, 0, 1);
		s6.bindTexture();
		//указываем модель которую хотим нарисовать
		st11.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(18, 4, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(0, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s7.bindTexture();
		//указываем модель которую хотим нарисовать
		st21.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-15, 20, z);
		//указываем текстуру, которую будем использовать
		glRotated(-70, 90, 0, 1);
		s7.bindTexture();
		//указываем модель которую хотим нарисовать
		st21.DrawObj();
		glPopMatrix();





		glPushMatrix();
		glTranslated(14, 10, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(60, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s5.bindTexture();
		//указываем модель которую хотим нарисовать
		st13.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(8, -19, z);
		//указываем текстуру, которую будем использовать
		glRotated(-90, 70, 0, 1);
		s5.bindTexture();
		//указываем модель которую хотим нарисовать
		st13.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(10, 5, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(180, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s6.bindTexture();
		//указываем модель которую хотим нарисовать
		st11.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(2, -12, z);
		//указываем текстуру, которую будем использовать
		glRotated(-50, -90, 0, 1);
		s6.bindTexture();
		//указываем модель которую хотим нарисовать
		st11.DrawObj();
		glPopMatrix();


		glPushMatrix();
		glTranslated(-12, -15, z);
		glScaled(0.4, 0.4, 0.4);
		glRotated(-30, 90, 0, 1);
		//указываем текстуру, которую будем использовать
		s7.bindTexture();
		//указываем модель которую хотим нарисовать
		st21.DrawObj();
		glPopMatrix();

		glPushMatrix();
		glScaled(0.4, 0.4, 0.4);
		glTranslated(-6, 10, z);
		//указываем текстуру, которую будем использовать
		glRotated(-90, -50, 0, 1);
		s7.bindTexture();
		//указываем модель которую хотим нарисовать
		st21.DrawObj();
		glPopMatrix();
	}

}

#pragma endregion

void Render(OpenGL *ogl)
{   
	
	tick_o = tick_n;
	tick_n = GetTickCount();
	Time += (tick_n - tick_o) / 1000.0;



	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);


	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	
	s[0].UseShader();

	//передача параметров в шейдер.  Шаг один - ищем адрес uniform переменной по ее имени. 
	int location = glGetUniformLocationARB(s[0].program, "light_pos");
	//Шаг 2 - передаем ей значение
	glUniform3fARB(location, light.pos.X(), light.pos.Y(),light.pos.Z());

	location = glGetUniformLocationARB(s[0].program, "Ia");
	glUniform3fARB(location, 0.2, 0.2, 0.2);

	location = glGetUniformLocationARB(s[0].program, "Id");
	glUniform3fARB(location, 1.0, 1.0, 1.0);

	location = glGetUniformLocationARB(s[0].program, "Is");
	glUniform3fARB(location, .7, .7, .7);


	location = glGetUniformLocationARB(s[0].program, "ma");
	glUniform3fARB(location, 0.2, 0.2, 0.1);

	location = glGetUniformLocationARB(s[0].program, "md");
	glUniform3fARB(location, 0.4, 0.65, 0.5);

	location = glGetUniformLocationARB(s[0].program, "ms");
	glUniform4fARB(location, 0.9, 0.8, 0.3, 25.6);

	location = glGetUniformLocationARB(s[0].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

	
	s[1].UseShader();
	
	Cloudw();
	

	if (flag == true)
	{	
		Style2();
	}
	else if (flag == false) {
		Style1();
	}


	if (shar==true && flag==true) {
		Shar1();
	}
	else if (shar == true && flag == false) {
		Shar2();
	}
	
	
	Shader::DontUseShaders();

	
	
}   //конец тела функции


bool gui_init = false;

//рисует интерфейс, вызывется после обычного рендера
void RenderGUI(OpenGL *ogl)
{
	
	Shader::DontUseShaders();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);
	

	glActiveTexture(GL_TEXTURE0);
	rec.Draw();


		
	Shader::DontUseShaders(); 



	
}

void resizeEvent(OpenGL *ogl, int newW, int newH)
{
	rec.setPosition(10, newH - 100 - 10);
}

