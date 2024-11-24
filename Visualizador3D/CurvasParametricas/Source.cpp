// Trabalho GrauA CG2024-2
// Nome: Rodrigo Fuelber Franke

// Código adaptado de https://github.com/fellowsheep/CG2024-2

#include <iostream>
#include <string>
#include <assert.h>

#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STL
#include <vector>

#include <random>
#include <algorithm>

//Classe gerenciadora de shaders
#include "Shader.h"

//STB_IMAGE
#include <stb_image.h>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);


// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1920, HEIGHT = 1080;

int modelo = 1;

//Variáveis globais da câmera
glm::vec3 cameraPos = glm::vec3(0.0f,0.0f,3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f,0.0,-1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f,1.0f,0.0f);


struct Curve
{
    std::vector<glm::vec3> controlPoints; // Pontos de controle da curva
    std::vector<glm::vec3> curvePoints;   // Pontos da curva
    glm::mat4 M;                          // Matriz dos coeficientes da curva
};


struct Object
{
	GLuint VAO; //Índice do buffer de geometria
	int nVertices; //nro de vértices
	glm::mat4 model; //matriz de transformações do objeto
    bool rotateX = false;
    bool rotateY = false;
    bool rotateZ = false;
    float angle = 0.0f;
    float escala = 1.0f;
	glm::vec3 movimento;

    float ka, kd, ks; //coeficientes de iluminação - material do objeto
    GLuint texID; //Identificador da textura carregada

   	bool segueCurva = false; // se o objeto esta seguindo uma curva
    int tipoCurva = -1;     // qual tipo de curva o obj esta fazendo
    int posicaoCurva = 0;   // index de posicao do obj na curva
};
std::vector<Object> objects;


//  leitores
void mtl_reader(string filePath, Object& objeto);
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 color);
GLuint loadTexture(string filePath, int &width, int &height);


// Outras funções
void generateHermiteCurvePoints(Curve &curve, int numPoints);
void generateBezierCurvePoints(Curve &curve, int numPoints);
void generateCatmullRomCurvePoints(Curve &curve, int numPoints);
std::vector<glm::vec3> generateHeartControlPoints(int numPoints = 20);
void generateGlobalBezierCurvePoints(Curve &curve, int numPoints);

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();


	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Visualizador 3D ----- GrauA", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}


	// Compilando e buildando o programa de shader
	Shader shader("phong/phong.vs","phong/phong.fs");


    glm::vec3 red = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 green = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 blue = glm::vec3(0.0f, 0.0f, 1.0f);


    int texWidth,texHeight;
    
    // carregar obj1 - Suzanne
	Object obj1;
    obj1.VAO = loadSimpleOBJ("Modelos3D/Suzannes/SuzanneHigh.obj", obj1.nVertices, blue);
    obj1.texID = loadTexture("Modelos3D/Suzannes/Suzanne.png", texWidth, texHeight);
    mtl_reader("Modelos3D/Suzannes/SuzanneHigh.mtl", obj1);

    objects.push_back(obj1);

    // carregar obj2 - Planeta
    Object obj2;
    obj2.VAO = loadSimpleOBJ("Modelos3D/Planetas/planeta.obj", obj2.nVertices, blue);
    obj2.texID = loadTexture("Modelos3D/Planetas/Terra.jpg", texWidth, texHeight);
    mtl_reader("Modelos3D/Planetas/planeta.mtl", obj2);

    objects.push_back(obj2);

    // carregar obj3 - Rato
    Object obj3;
    obj3.VAO = loadSimpleOBJ("Modelos3D/aratwearingabackpack/obj/model.obj", obj3.nVertices, blue);
    obj3.texID = loadTexture("Modelos3D/aratwearingabackpack/textures/texture_1.jpeg", texWidth, texHeight);
    mtl_reader("Modelos3D/aratwearingabackpack/material/model.mtl", obj3);

    objects.push_back(obj3);


    float lastTime = 0.0;
    float FPS = 60.0;
    float angle = 0.0;

    // Estrutura para armazenar a curva de Bézier e pontos de controle
    Curve curvaBezier;
    Curve curvaCatmullRom;
    Curve curvaHermite;

    std::vector<glm::vec3> controlPoints = generateHeartControlPoints();

    curvaBezier.controlPoints = controlPoints;

	// Para os pontos de controle da Catmull Rom precisamos duplicar o primeiro e o último
    curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[0]);
    for (int i = 0; i < curvaBezier.controlPoints.size(); i++)
    {
        curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[i]);
    }
    curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[curvaBezier.controlPoints.size() - 1]);


    // pontos de controle hermite
    curvaHermite.controlPoints.push_back(glm::vec3(-3.0f, -1.0f, 0.0f));
    curvaHermite.controlPoints.push_back(glm::vec3(3.0f, 2.0f, 0.0f));
    curvaHermite.controlPoints.push_back(glm::vec3(2.0f, 3.0f, 0.0f)); 
    curvaHermite.controlPoints.push_back(glm::vec3(-1.0f, -2.0f, 0.0f));





    // Gerar pontos da curva de Bézier
    int numCurvePoints = 15; // Quantidade de pontos por segmento na curva
    generateGlobalBezierCurvePoints(curvaBezier, numCurvePoints);
    generateBezierCurvePoints(curvaBezier, numCurvePoints);
    generateCatmullRomCurvePoints(curvaCatmullRom, numCurvePoints);



    generateHermiteCurvePoints(curvaHermite, numCurvePoints*14);




	glUseProgram(shader.ID);

	//Matriz de modelo
	glm::mat4 model = glm::mat4(1); //matriz identidade;
	GLint modelLoc = glGetUniformLocation(shader.ID, "model");
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//Matriz de view
	glm::mat4 view = glm::lookAt(cameraPos,glm::vec3(0.0f,0.0f,0.0f),cameraUp);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
	//Matriz de projeção
	glm::mat4 projection = glm::perspective(glm::radians(39.6f),(float)WIDTH/HEIGHT,0.1f,100.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glEnable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);


	// exponente de especularidade (controla a nitidez dos reflexos especulares)
	shader.setFloat("q", 100.0);

	//Propriedades da fonte de luz
	shader.setVec3("lightPos",-2.0, 10.0, 3.0);
	shader.setVec3("lightColor",1.0, 1.0, 1.0);




	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{


		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

        // Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	    int width, height;
	    glfwGetFramebufferSize(window, &width, &height);
	    glViewport(0, 0, width, height);

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);


        // Incrementando o índice do frame apenas quando fechar a taxa de FPS desejada
        float now = glfwGetTime();
        float dt = now - lastTime;
        if (dt >= 1 / FPS) {
            for (Object& objeto : objects) {
                // Verifica se o objeto segue alguma curva
                if (objeto.segueCurva && objeto.tipoCurva != -1) {
                    Curve* curvaAtual = nullptr;

                    // Determina qual tipo de curva o objeto vai fazer
                    if (objeto.tipoCurva == 0) {
                        curvaAtual = &curvaCatmullRom;
                    } else if (objeto.tipoCurva == 1) {
                        curvaAtual = &curvaBezier;
                    } else if (objeto.tipoCurva == 2) {
                        curvaAtual = &curvaHermite;
                    }

                    if (curvaAtual) {
                         // incrementando ciclicamente o indice do Frame
                        int index = (objeto.posicaoCurva + 1) % curvaAtual->curvePoints.size();
                        objeto.posicaoCurva = index;

                        // atualiza a posição do objeto pela  curva
                        glm::vec3 nextPos = curvaAtual->curvePoints[index];
                        glm::vec3 currentPos = curvaAtual->curvePoints[objeto.posicaoCurva];
                        objeto.movimento = currentPos;

                        glm::vec3 dir = glm::normalize(nextPos - currentPos);
                        angle = atan2(dir.y, dir.x) + glm::radians(-90.0f);
                    }
                }
            }
            lastTime = now;
        } 


        for (int i = 0; i < objects.size(); ++i)
        {
            Object& objeto = objects[i];
            

            // coeficiente de reflexao ambiente (quanto da luz ambiente é refletida pelo objeto)
            shader.setFloat("ka", objeto.ka);

            // coeficiente de reflexao difusa (fração da luz incidente a ser refletida/espalhada na superfície)
            shader.setFloat("kd", objeto.kd);

            // coeficiente de reflexao especular (circulos brilhantes que aparecem em superficies)
            shader.setFloat("ks", objeto.ks);
            
            objeto.model = glm::mat4(1); // matriz identidade 

            objeto.model = glm::translate(objeto.model, objeto.movimento);

            
            // aplica a rotação pro objeto
            objeto.angle = (GLfloat)glfwGetTime();
            if (objeto.rotateX)
            {
                objeto.model = glm::rotate(objeto.model, objeto.angle, glm::vec3(1.0f, 0.0f, 0.0f));
            }
            else if (objeto.rotateY)
            {
                objeto.model = glm::rotate(objeto.model, objeto.angle, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            else if (objeto.rotateZ)
            {
                objeto.model = glm::rotate(objeto.model, objeto.angle, glm::vec3(0.0f, 0.0f, 1.0f));
            }
				

			// aplica a escala pro objeto
            objeto.model = glm::scale(objeto.model, glm::vec3(objeto.escala, objeto.escala, objeto.escala));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(objeto.model));
			//Atualizar a matriz de view
			//Matriz de view
			glm::mat4 view = glm::lookAt(cameraPos,cameraPos + cameraFront,cameraUp);
			glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));


			//Propriedades da câmera
			shader.setVec3("cameraPos",cameraPos.x, cameraPos.y, cameraPos.z);


			// Chamada de desenho - drawcall
			// Poligono Preenchido - GL_TRIANGLES
            glBindVertexArray(objeto.VAO);
            glBindTexture(GL_TEXTURE_2D,objeto.texID);
            glDrawArrays(GL_TRIANGLES, 0, objeto.nVertices);
        }

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}


	// Pede pra OpenGL desalocar os buffers
	for(Object objeto : objects){
		glDeleteVertexArrays(1, &objeto.VAO);
	}
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);



	// Pega o numero selecionado pelo usuario
	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && action == GLFW_PRESS)
	{
		modelo = key - GLFW_KEY_0;
	}


    if (key == GLFW_KEY_0 && action == GLFW_PRESS) // reseta a posição
    {
        objects[modelo - 1].movimento = glm::vec3(0.0f, 0.0f, 0.0f);     
        objects[modelo - 1].segueCurva = false;
        objects[modelo - 1].tipoCurva = -1;
    }


	// Aplica a rotacao no eixo
	if (key == GLFW_KEY_X && action == GLFW_PRESS) // Eixo X
    {
        objects[modelo - 1].rotateX = true;
        objects[modelo - 1].rotateY = false;
        objects[modelo - 1].rotateZ = false;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS) // Eixo Y
    {
        objects[modelo - 1].rotateX = false;
        objects[modelo - 1].rotateY = true;
        objects[modelo - 1].rotateZ = false;
    }

    if (key == GLFW_KEY_Z && action == GLFW_PRESS) // Eixo Z
    {
        objects[modelo - 1].rotateX = false;
        objects[modelo - 1].rotateY = false;
        objects[modelo - 1].rotateZ = true;
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS) // Para a rotacao
    {
        objects[modelo - 1].rotateX = false;
        objects[modelo - 1].rotateY = false;
        objects[modelo - 1].rotateZ = false;
    }


	// Movimenta a camera
	float cameraSpeed = 0.3f;

	if ((key == GLFW_KEY_W) && action == GLFW_PRESS) // Frente
	{
		cameraPos += cameraSpeed * cameraFront;
	}
	if ((key == GLFW_KEY_S) && action == GLFW_PRESS) // Tras
	{
		cameraPos -= cameraSpeed * cameraFront;
	}
	if ((key == GLFW_KEY_A) && action == GLFW_PRESS) // Esquerda
	{
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if ((key == GLFW_KEY_D) && action == GLFW_PRESS)  // Direita
	{
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}

    if ((key == GLFW_KEY_SPACE) && action == GLFW_PRESS) // Sobe
    {
        cameraPos += cameraSpeed * cameraUp;
    }
    if ((key == GLFW_KEY_LEFT_CONTROL) && action == GLFW_PRESS) // Desce
    {
        cameraPos -= cameraSpeed * cameraUp;
    }

	


	// Altera a escala do Objeto
	float escalaSize = 0.1f;

    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) // Aumenta
    {
        objects[modelo - 1].escala += escalaSize;
    }

    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) // Diminui
    {
        objects[modelo - 1].escala = glm::max(objects[modelo - 1].escala - escalaSize, 0.1f);
    }




	// Movimenta o Objeto
	float objetoSpeed =  0.2f;

    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) // Esquerda
    {
        objects[modelo - 1].movimento.x -= objetoSpeed;
    }
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) // Direita
    {
        objects[modelo - 1].movimento.x += objetoSpeed;
    }

    if (key == GLFW_KEY_UP && action == GLFW_PRESS) // Sobe
    {
        objects[modelo - 1].movimento.y += objetoSpeed;
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) // Desce
    {
        objects[modelo - 1].movimento.y -= objetoSpeed;
    }

    if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS) // Frente
    {
        objects[modelo - 1].movimento.z += objetoSpeed;
    }
    if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS) // Tras
    {
        objects[modelo - 1].movimento.z -= objetoSpeed;
    }


    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        objects[modelo - 1].segueCurva = true;
        objects[modelo - 1].tipoCurva = 0; // Catmull-Rom
    }
    if (key == GLFW_KEY_B && action == GLFW_PRESS) {
        objects[modelo - 1].segueCurva = true;
        objects[modelo - 1].tipoCurva = 1; // Bézier
    }
    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        objects[modelo - 1].segueCurva = true;
        objects[modelo - 1].tipoCurva = 2; // Hermite
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        objects[modelo - 1].segueCurva = false;
        objects[modelo - 1].tipoCurva = -1; // Para de seguir a curva
    }


}


//      ----------- LEITOR MTL/OBJ/TEXTURA -----------

void mtl_reader(string filePath, Object& objeto) {
    std::ifstream arqEntrada(filePath);

    objeto.kd = 0.5f; // valor caso mtl não tenha o valor de kd
    std::string line;

    while (std::getline(arqEntrada, line)) {
        std::istringstream iss(line);
        std::string word;
        iss >> word;

        if (word == "Ka") { // coeficiente ambiente
            float ka1, ka2, ka3;
            iss >> ka1 >> ka2 >> ka3;
            objeto.ka = (ka1 + ka2 + ka3) / 3.0f;


        } else if (word == "Kd") { // coeficiente difuso
            float kd1, kd2, kd3;
            iss >> kd1 >> kd2 >> kd3;
            objeto.kd = (kd1 + kd2 + kd3) / 3.0f;


        } else if (word == "Ks") { // coeficiente especular
            float ks1, ks2, ks3;
            iss >> ks1 >> ks2 >> ks3;
            objeto.ks = (ks1 + ks2 + ks3) / 3.0f;
        }
    }

    arqEntrada.close();
}

int loadSimpleOBJ(string filePath, int &nVertices, glm::vec3 color)
{
	vector <glm::vec3> vertices;
	vector <glm::vec2> texCoords;
	vector <glm::vec3> normals;
	vector <GLfloat> vBuffer;

	ifstream arqEntrada;

	arqEntrada.open(filePath.c_str());
	if (arqEntrada.is_open())
	{
		//Fazer o parsing
		string line;
		while (!arqEntrada.eof())
		{
			getline(arqEntrada,line);
			istringstream ssline(line);
			string word;
			ssline >> word;
			if (word == "v")
			{
				glm::vec3 vertice;
				ssline >> vertice.x >> vertice.y >> vertice.z;
				vertices.push_back(vertice);

			}
			if (word == "vt")
			{
				glm::vec2 vt;
				ssline >> vt.s >> vt.t;
				texCoords.push_back(vt);

			}
			if (word == "vn")
			{
				glm::vec3 normal;
				ssline >> normal.x >> normal.y >> normal.z;
				normals.push_back(normal);

			}
			else if (word == "f")
			{
				while (ssline >> word) 
				{
					int vi, ti, ni;
					istringstream ss(word);
    				std::string index;

    				// Pega o índice do vértice
    				std::getline(ss, index, '/');
    				vi = std::stoi(index) - 1;  // Ajusta para índice 0

    				// Pega o índice da coordenada de textura
    				std::getline(ss, index, '/');
    				ti = std::stoi(index) - 1;

    				// Pega o índice da normal
    				std::getline(ss, index);
    				ni = std::stoi(index) - 1;

					//Recuperando os vértices do indice lido
					vBuffer.push_back(vertices[vi].x);
					vBuffer.push_back(vertices[vi].y);
					vBuffer.push_back(vertices[vi].z);
					
					//Atributo cor
					vBuffer.push_back(color.r);
					vBuffer.push_back(color.g);
					vBuffer.push_back(color.b);

					//Atributo coordenada de textura
					vBuffer.push_back(texCoords[ti].s);
					vBuffer.push_back(texCoords[ti].t);

					//Atributo vetor normal
					vBuffer.push_back(normals[ni].x);
					vBuffer.push_back(normals[ni].y);
					vBuffer.push_back(normals[ni].z);
    			}
				
			}
		}

		arqEntrada.close();

		cout << "Gerando o buffer de geometria..." << endl;
		GLuint VBO, VAO;

	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 
	
	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//Atributo coordenada de textura - s, t
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6*sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	//Atributo vetor normal - x, y, z
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8*sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	nVertices = vBuffer.size() / 2;
	return VAO;

	}
	else
	{
		cout << "Erro ao tentar ler o arquivo " << filePath << endl;
		return -1;
	}
}

GLuint loadTexture(string filePath, int &width, int &height)
{
	GLuint texID; // id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // assume que é 4 canais png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}


//      ----------- CURVAS PARAMETRICAS -----------

void initializeBernsteinMatrix(glm::mat4 &matrix){

    matrix[0] = glm::vec4(-1.0f, 3.0f, -3.0f, 1.0f); // Primeira coluna
    matrix[1] = glm::vec4(3.0f, -6.0f, 3.0f, 0.0f);  // Segunda coluna
    matrix[2] = glm::vec4(-3.0f, 3.0f, 0.0f, 0.0f);  // Terceira coluna
    matrix[3] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);   // Quarta coluna

}

void initializeCatmullRomMatrix(glm::mat4 &matrix)
{

    matrix[0] = glm::vec4(-0.5f, 1.5f, -1.5f, 0.5f); // Primeira linha
    matrix[1] = glm::vec4(1.0f, -2.5f, 2.0f, -0.5f); // Segunda linha
    matrix[2] = glm::vec4(-0.5f, 0.0f, 0.5f, 0.0f);  // Terceira linha
    matrix[3] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);   // Quarta linha
}

void initializeHermiteMatrix(glm::mat4 &matrix) {
    matrix[0] = glm::vec4(2.0f, -2.0f, 1.0f, 1.0f);
    matrix[1] = glm::vec4(-3.0f, 3.0f, -2.0f, -1.0f);
    matrix[2] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    matrix[3] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
}

void generateHermiteCurvePoints(Curve &curve, int numPoints)
{
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    initializeHermiteMatrix(curve.M);

    float piece = 1.0 / (float)numPoints; // Intervalo entre os pontos
    float t;

    for (int i = 0; i < curve.controlPoints.size(); i += 4)
    {
        glm::vec3 P0 = curve.controlPoints[i];
        glm::vec3 P1 = curve.controlPoints[i + 3];
        glm::vec3 P2 = curve.controlPoints[i + 1] - P0; 
        glm::vec3 P3 = curve.controlPoints[i + 2] - P1; 

        // Gera pontos para o segmento atual
        for (int j = 0; j <= numPoints; j++)
        {
            t = j * piece;

            // Vetor t para Hermite
            glm::vec4 T(t * t * t, t * t, t, 1);

            // Matriz G contendo os pontos e tangentes
            glm::mat4x3 G(P0, P1, P2, P3);

            // Calcula o ponto da curva
            glm::vec3 point = G * curve.M * T;

            // Adiciona o ponto gerado à curva
            curve.curvePoints.push_back(point);
        }
    }
}

void generateBezierCurvePoints(Curve &curve, int numPoints)
{
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    initializeBernsteinMatrix(curve.M);
    // Calcular os pontos ao longo da curva com base em Bernstein
    // Loop sobre os pontos de controle em grupos de 4

    float piece = 1.0 / (float)numPoints;
    float t;
    for (int i = 0; i < curve.controlPoints.size() - 3; i += 3)
    {

        // Gera pontos para o segmento atual
        for (int j = 0; j < numPoints; j++)
        {
            t = j * piece;

            // Vetor t para o polinômio de Bernstein
            glm::vec4 T(t * t * t, t * t, t, 1);

            glm::vec3 P0 = curve.controlPoints[i];
            glm::vec3 P1 = curve.controlPoints[i + 1];
            glm::vec3 P2 = curve.controlPoints[i + 2];
            glm::vec3 P3 = curve.controlPoints[i + 3];

            glm::mat4x3 G(P0, P1, P2, P3);

            // Calcula o ponto da curva multiplicando tVector, a matriz de Bernstein e os pontos de controle
            glm::vec3 point = G * curve.M * T;

            curve.curvePoints.push_back(point);
        }
    }
}

void generateCatmullRomCurvePoints(Curve &curve, int numPoints)
{
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    initializeCatmullRomMatrix(curve.M);
    // Calcular os pontos ao longo da curva com base em Bernstein
    // Loop sobre os pontos de controle em grupos de 4

    float piece = 1.0 / (float)numPoints;
    float t;
    for (int i = 0; i < curve.controlPoints.size() - 3; i++)
    {

        // Gera pontos para o segmento atual
        for (int j = 0; j < numPoints; j++)
        {
            t = j * piece;

            // Vetor t para o polinômio de Bernstein
            glm::vec4 T(t * t * t, t * t, t, 1);

            glm::vec3 P0 = curve.controlPoints[i];
            glm::vec3 P1 = curve.controlPoints[i + 1];
            glm::vec3 P2 = curve.controlPoints[i + 2];
            glm::vec3 P3 = curve.controlPoints[i + 3];

            glm::mat4x3 G(P0, P1, P2, P3);

            // Calcula o ponto da curva multiplicando tVector, a matriz de Bernstein e os pontos de controle
            glm::vec3 point = G * curve.M * T;
            curve.curvePoints.push_back(point);
        }
    }
}

std::vector<glm::vec3> generateHeartControlPoints(int numPoints)
{
    std::vector<glm::vec3> controlPoints;

    // Define o intervalo para t: de 0 a 2 * PI, dividido em numPoints
    float step = 2 * 3.14159 / (numPoints - 1);

    for (int i = 0; i < numPoints - 1; i++)
    {
        float t = i * step;

        // Calcula x(t) e y(t) usando as fórmulas paramétricas
        float x = 16 * pow(sin(t), 3);
        float y = 13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t);

        // Normaliza os pontos para mantê-los dentro de [-1, 1] no espaço 3D
        x /= 16.0f; // Dividir por 16 para normalizar x entre -1 e 1
        y /= 16.0f; // Dividir por 16 para normalizar y aproximadamente entre -1 e 1
        y += 0.15;
        // Adiciona o ponto ao vetor de pontos de controle
        controlPoints.push_back(glm::vec3(x, y, 0.0f));
    }
    controlPoints.push_back(controlPoints[0]);

    return controlPoints;
}

void generateGlobalBezierCurvePoints(Curve &curve, int numPoints)
{
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    int n = curve.controlPoints.size() - 1; // Grau da curva
    float t;
    float piece = 1.0f / (float)numPoints;

    for (int j = 0; j <= numPoints; ++j)
    {
        t = j * piece;
        glm::vec3 point(0.0f); // Ponto na curva

        // Calcula o ponto da curva usando a fórmula de Bernstein
        for (int i = 0; i <= n; ++i)
        {
            // Coeficiente binomial
            float binomialCoeff = (float)(tgamma(n + 1) / (tgamma(i + 1) * tgamma(n - i + 1)));
            // Polinômio de Bernstein
            float bernsteinPoly = binomialCoeff * pow(1 - t, n - i) * pow(t, i);
            // Soma ponderada dos pontos de controle
            point += bernsteinPoly * curve.controlPoints[i];
        }

        curve.curvePoints.push_back(point);
    }
}
