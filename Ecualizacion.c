#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <omp.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb-master/stb_image_write.h"
#include "stb-master/stb_image.h"

double tiemposMostradosS[4];
double tiemposMostradosP[4];

//FUNCIONES SECUENCIALES
void crearCarpeta(char *);
void LlenadoEnCeros(int*,int);
char* concatenador(char*,char*);
unsigned char* unCanal(unsigned char*,int);
unsigned char* tresCanales(unsigned char*, int, int);
void generarCSV(char*, unsigned char*,unsigned char*, int);
void ImaSecuencial(char*,int,int,int,char*,unsigned char*, double);
void generarMetricasCompletas(char*, char*,int, int, int, double*, double*);
unsigned char* tresCanalesFucion(unsigned char*,unsigned char*,unsigned char*,int);
void generarMetricas(char*, char*,int , int , int , double , double, double, double);

//FUNCIONES PARALELAS
unsigned char* unCanalP(unsigned char*,int);
unsigned char* tresCanalesP(unsigned char*, int, int);
void ImaParalelo(char*,int,int,int,char*,unsigned char*, double);
unsigned char* tresCanalesFucionP(unsigned char*,unsigned char*,unsigned char*,int);

int main(int argc, char const *argv[]){
	if(argc < 3){
		printf("\nFormato de ingreso incorrecto\n");
		printf("FORMA CORRECTA:\n");
		printf("./programa NombreImagen NombreCarpeta\n");
		printf("NombreImagen: incluya extencion --> ima.jpg\n");
		printf("NombreCarpeta: si no existe coloque un punto --> .\n\n");
		return 0;
	}

	char *NombreImagen = argv[1];
	char *NombreCarpeta = argv[2];

	if(NombreCarpeta=="."){
		NombreCarpeta = "";
	}
	double inicio = omp_get_wtime();		
	char *Carpeta = concatenador(NombreCarpeta,"/");  
	char *ruta = concatenador(Carpeta,NombreImagen);	
	int width, height, channels;
	unsigned char *srcIma = stbi_load(ruta, &width, &height, &channels, 0);
	double final = omp_get_wtime();
	double tiempoDeCarga = final- inicio;
	if(srcIma == NULL){
		printf("No se pudo cargar la imagen %s \n", ruta);
		return 0;
	}

	char *CarpetaImagenGenerada = concatenador("Carpeta para ", NombreImagen);
	crearCarpeta(CarpetaImagenGenerada);
	char *Imac =concatenador("./",CarpetaImagenGenerada);
	Imac = concatenador(Imac,"/");


	ImaSecuencial(NombreImagen,channels,width,height,Imac,srcIma,tiempoDeCarga);
	ImaParalelo(NombreImagen,channels,width,height,Imac,srcIma,tiempoDeCarga);
	generarMetricasCompletas(NombreImagen,concatenador(Imac,"MetricasCompletas"), height, width, channels,tiemposMostradosS,tiemposMostradosP);
	return 0;
}
char* concatenador(char *cad1,char *cad2){	
	char *src = (char *) malloc(strlen(cad1)+strlen(cad2)+1);
	sprintf(src, "%s%s",cad1, cad2);
	return src;
}

void LlenadoEnCeros(int *arr,int tamano){
	for(int i = 0; i < tamano;i++){
		arr[i]=0;
	}
}

unsigned char* unCanal(unsigned char *srcIma, int tamano){
	//creamos el arreglo de frecuencias
	int frecuencias[256];
	int frecuenciasAcumulado[256];
	int frecuenciasAcumuladoEqualizado[256];
	unsigned char *srcImaEqualizada = malloc(sizeof(unsigned char) * tamano);

	LlenadoEnCeros(frecuencias,256);
	LlenadoEnCeros(frecuenciasAcumulado,256);
	LlenadoEnCeros(frecuenciasAcumuladoEqualizado,256);
	
	//llenamos el arreglo de frecuencias
	for(int i = 0; i < tamano;i++){
		frecuencias[srcIma[i]]+=1;
	}	

	//generamos el cdf ---> Arreglo de frecuencias acumulado
	int min_cdf=0;
	frecuenciasAcumulado[0]=frecuencias[0];
	for(int i = 1; i < 256;i++){
		if(min_cdf==0){
			min_cdf = frecuenciasAcumulado[i-1];		
		}
		frecuenciasAcumulado[i]=frecuenciasAcumulado[i-1]+frecuencias[i];
	}

	//generamos el nuevo arreglo de frecuencias

	for(int i = 0; i < 256; i++){

		frecuenciasAcumuladoEqualizado[i] =round(((((double)frecuenciasAcumulado[i]-min_cdf)/(tamano-min_cdf))*254)+1); 	
	}

	for(int i=0; i < tamano;i++){
		srcImaEqualizada[i] =  frecuenciasAcumuladoEqualizado[srcIma[i]];
	}

	return srcImaEqualizada;
}

unsigned char* tresCanales(unsigned char *srcIma, int tamano, int c){
	//creamos el arreglo de frecuencias	
	unsigned char *srcImaAux = malloc(sizeof(unsigned char) * tamano);
	for(int i=0; i < tamano;i++){
		srcImaAux[i]=srcIma[i*3+c];
	}
	srcImaAux=unCanal(srcImaAux,tamano);
	return srcImaAux;	
}

unsigned char* tresCanalesFucion(unsigned char *src1,unsigned char *src2,unsigned char *src3,int tamano){
	
	unsigned char *src4 = malloc(sizeof(unsigned char) * tamano*3);
	for(int i=0;i<tamano; i++){
		src4[i*3]=src1[i];
		src4[i*3+1]=src2[i];
		src4[i*3+2]=src3[i];
	}
	return src4;
}

void crearCarpeta(char *ruta){

	int creado = mkdir(ruta, 0777);
	if(creado!=0){
		printf("%d -- Error al generar la %s, puede que ya exista\n",creado, ruta);
	}

}

void generarCSV(char *nombre, unsigned char *srcIma,unsigned char *srcImaEqualizada, int tamano){
	nombre=concatenador(nombre,".csv");
	int frecuencias[256];
	int frecuencias1[256];
	LlenadoEnCeros(frecuencias,256);
	LlenadoEnCeros(frecuencias1,256);
	for(int i=0; i<tamano;i++){
		frecuencias[srcIma[i]]+=1;
		frecuencias1[srcImaEqualizada[i]]+=1;
	}

	FILE *CSV = fopen(nombre, "w+");

	// Header
    fprintf(CSV, "%s,%s,%s \n", "Valor", "Original", "Equalizado");

	for(int i = 0; i < 256; i++){
		fprintf(CSV, "%d,%d,%d \n", i, frecuencias[i],frecuencias1[i]);
	}

	fclose(CSV);
}

void ImaSecuencial(char *NombreImagen,int channels,int width,int height,char *Imac,unsigned char *srcIma,double tiempoDeCarga){
	char *NombreArchivo = concatenador(Imac,"MetricasSecuencial");
	double tiempoDeEjecucion = 0;
	double tiempoDeGeneracion = 0;
	double tiempoDeGeneracionCSV = 0;
	double inicio, final;
	
	if(channels==3){
		inicio = omp_get_wtime();
		int tamano = height*width;
		unsigned char *ima0 = tresCanales(srcIma,tamano,0);
		unsigned char *ima1 = tresCanales(srcIma,tamano,1);
		unsigned char *ima2 = tresCanales(srcIma,tamano,2);
		unsigned char *ima = tresCanalesFucion(ima0, ima1, ima2,tamano);
		final = omp_get_wtime();
		tiempoDeEjecucion = final- inicio;

		inicio = omp_get_wtime();
		char *Imac1 = concatenador(Imac,"PimerCanalSecuencial-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, 1, ima0, 100);

		Imac1 = concatenador(Imac,"SegundoCanalSecuencial-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, 1, ima1, 100);

		Imac1 = concatenador(Imac,"TercerCanalSecuencial-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, 1, ima2, 100);

		Imac1 = concatenador(Imac,"EcualizacionSecuencial-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, channels, ima, 100);
		final = omp_get_wtime();		
		tiempoDeGeneracion = final- inicio;

		inicio = omp_get_wtime();
		Imac1 = concatenador(Imac,"Resumen de Histograma Secuencial");		
		generarCSV(Imac1, srcIma, ima, tamano*3);
		final = omp_get_wtime();		
		tiempoDeGeneracionCSV = final- inicio;

		free(Imac1);
		free(ima);
		free(ima0);
		free(ima1);
		free(ima2);

	}else if(channels==1){

		inicio = omp_get_wtime();
		int tamano = height*width;
		unsigned char *ima = unCanal(srcIma,tamano);
		final = omp_get_wtime();
		tiempoDeEjecucion = final- inicio;

		inicio = omp_get_wtime();
		char *Imac1 = concatenador(Imac,"EcualizacionSecuencial-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, channels, ima, 100);
		final = omp_get_wtime();		
		tiempoDeGeneracion = final- inicio;

		inicio = omp_get_wtime();
		Imac1 = concatenador(Imac,"Resumen de Histograma Secuencial");
		generarCSV(Imac1, srcIma, ima, tamano);
		final = omp_get_wtime();		
		tiempoDeGeneracionCSV = final- inicio;
		
		free(ima);
		free(Imac1);

	}else{
		printf("Numero de canales no contemplado\n");

	}
	generarMetricas(NombreImagen,NombreArchivo,height,width,channels,tiempoDeEjecucion,tiempoDeCarga,tiempoDeGeneracion,tiempoDeGeneracionCSV);	
	
	
	tiemposMostradosS[0]=tiempoDeEjecucion;
	tiemposMostradosS[1]=tiempoDeCarga;
	tiemposMostradosS[2]=tiempoDeGeneracion;
	tiemposMostradosS[3]=tiempoDeGeneracionCSV;
}

void generarMetricas(char *NombreImagen,char *NombreArchivo,int alto, int ancho, int canales, double tiempoDeEjecucion, double tiempoDeCarga,double tiempoDeGeneracion, double tiempoDeGeneracionCSV){
	NombreArchivo=concatenador(NombreArchivo,".txt");

	FILE *Metricas = fopen(NombreArchivo, "w+");
	fprintf(Metricas, "Metricas de %s: \n",NombreImagen);
	fprintf(Metricas, "\tMedidas de imagen:\n");
	fprintf(Metricas, "\tAlto: %d\n",alto);
	fprintf(Metricas, "\tAncho: %d\n",ancho);
	fprintf(Metricas, "\tCanales: %d\n",canales);
	fprintf(Metricas, "\tTamano: %d\n",canales*alto*ancho);
	fprintf(Metricas, "Tiempo de ejecucion: %f\n",tiempoDeEjecucion);
	fprintf(Metricas, "Tiempo de carga: %f\n",tiempoDeCarga);
	fprintf(Metricas, "Tiempo de generacion: %f\n",tiempoDeGeneracion);
	fprintf(Metricas, "Tiempo de generacion csv: %f\n",tiempoDeGeneracionCSV);
	fclose(Metricas);
}
void generarMetricasCompletas(char *NombreImagen,char *NombreArchivo,int alto, int ancho, int canales, double* tiemposMostradosSecuencial,double* tiemposMostradosParalelo){
	NombreArchivo=concatenador(NombreArchivo,".txt");

	FILE *Metricas = fopen(NombreArchivo, "w+");
	fprintf(Metricas, "Metricas de %s: \n",NombreImagen);
	fprintf(Metricas, "\tMedidas de imagen:\n");
	fprintf(Metricas, "\tAlto: %d\n",alto);
	fprintf(Metricas, "\tAncho: %d\n",ancho);
	fprintf(Metricas, "\tCanales: %d\n",canales);
	fprintf(Metricas, "\tTamano: %d\n",canales*alto*ancho);
	fprintf(Metricas, "Tiempo de ejecucion en serie: %f\n",tiemposMostradosSecuencial[0]);
	fprintf(Metricas, "Tiempo de ejecucion en paralelo: %f\n",tiemposMostradosParalelo[0]);
	fprintf(Metricas, "Numero de procesadores: %d\n",omp_get_num_procs());
	fprintf(Metricas, "Speedup: %f\n",tiemposMostradosSecuencial[0]/tiemposMostradosParalelo[0]);
	fprintf(Metricas, "Eficiencia: %f\n",(tiemposMostradosSecuencial[0]/tiemposMostradosParalelo[0])/omp_get_num_procs());
	fprintf(Metricas, "Overhead: %f\n",(tiemposMostradosParalelo[0]-tiemposMostradosSecuencial[0])/omp_get_num_procs());
	fprintf(Metricas, "Tiempo de carga: %f\n",(tiemposMostradosSecuencial[1]+tiemposMostradosSecuencial[1])/2);
	fprintf(Metricas, "Tiempo de generacion promedio: %f\n",(tiemposMostradosSecuencial[2]+tiemposMostradosSecuencial[2])/2);
	fprintf(Metricas, "Tiempo de generacion csv promedio: %f\n",(tiemposMostradosSecuencial[3]+tiemposMostradosSecuencial[3])/2);
	fclose(Metricas);
	//Mostramos las metricas en la consola 
	printf("Metricas de %s: \n",NombreImagen);
	printf("\tMedidas de imagen:\n");
	printf("\tAlto: %d\n",alto);
	printf("\tAncho: %d\n",ancho);
	printf("\tCanales: %d\n",canales);
	printf("\tTamano: %d\n",canales*alto*ancho);
	printf("Tiempo de ejecucion en serie: %f\n",tiemposMostradosSecuencial[0]);
	printf("Tiempo de ejecucion en paralelo: %f\n",tiemposMostradosParalelo[0]);
	printf("Numero de procesadores: %d\n",omp_get_num_procs());
	printf("Speedup: %f\n",tiemposMostradosSecuencial[0]/tiemposMostradosParalelo[0]);
	printf("Eficiencia: %f\n",(tiemposMostradosSecuencial[0]/tiemposMostradosParalelo[0])/omp_get_num_procs());
	printf("Overhead: %f\n",(tiemposMostradosParalelo[0]-tiemposMostradosSecuencial[0])/omp_get_num_procs());
	printf("Tiempo de carga: %f\n",(tiemposMostradosSecuencial[1]+tiemposMostradosSecuencial[1])/2);
	printf("Tiempo de generacion promedio: %f\n",(tiemposMostradosSecuencial[2]+tiemposMostradosSecuencial[2])/2);
	printf("Tiempo de generacion csv promedio: %f\n",(tiemposMostradosSecuencial[3]+tiemposMostradosSecuencial[3])/2);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char* unCanalP(unsigned char *srcIma, int tamano){
	//creamos el arreglo de frecuencias
	int frecuencias[256];
	int frecuenciasAcumulado[256];
	int frecuenciasAcumuladoEqualizado[256];
	int min_cdf = 0;	
	unsigned char *srcImaEqualizada = malloc(sizeof(unsigned char) * tamano);
	#pragma omp parallel
	{
		#pragma omp for
		for(int i = 0; i < 256;i++){
			frecuencias[i]=0;
			frecuenciasAcumulado[i]=0;		
			frecuenciasAcumuladoEqualizado[i]=0;
		}			
		#pragma omp for reduction(+ : frecuencias)
		for(int i = 0; i < tamano;i++){
		  frecuencias[srcIma[i]]+=1;
		}
		#pragma omp single
		{	
			//generamos el cdf ---> Arreglo de frecuencias acumulado
			//mejor no paralelizarlo
			frecuenciasAcumulado[0]=frecuencias[0];
			for(int i = 1; i < 256;i++){
				if(min_cdf==0){
					min_cdf = frecuenciasAcumulado[i-1];		
				}
				frecuenciasAcumulado[i]=frecuenciasAcumulado[i-1]+frecuencias[i];
			}
		}
		//generamos el nuevo arreglo de frecuencias
		#pragma omp for
		for(int i = 0; i < 256; i++){
			frecuenciasAcumuladoEqualizado[i] =(int) round((double)(frecuenciasAcumulado[i]-min_cdf)/(double)(tamano-min_cdf)*254)+1; 	
		}
		#pragma omp for
		for(int i=0; i < tamano;i++){
			srcImaEqualizada[i] =  frecuenciasAcumuladoEqualizado[srcIma[i]];
		}
	}
	return srcImaEqualizada;
}

unsigned char* tresCanalesP(unsigned char *srcIma, int tamano, int c){
	unsigned char *srcImaEqualizada = malloc(sizeof(unsigned char) * tamano);
	unsigned char *srcImaAux = malloc(sizeof(unsigned char) * tamano);
	int min_cdf=0;	
	int frecuencias[256];
	int frecuenciasAcumulado[256];
	int frecuenciasAcumuladoEqualizado[256];
	#pragma omp parallel
	{
		#pragma omp for
		for(int i=0; i < tamano;i++){
			srcImaAux[i]=srcIma[i*3+c];
		}

		#pragma omp for
		for(int i = 0; i < 256;i++){
			frecuencias[i]=0;
			frecuenciasAcumulado[i]=0;		
			frecuenciasAcumuladoEqualizado[i]=0;
		}
		//llenamos el arreglo de frecuencias	
		#pragma omp for reduction(+ : frecuencias)
		for(int i = 0; i < tamano;i++){
		  frecuencias[srcImaAux[i]]+=1;
		}
		#pragma omp single
		{	
			//generamos el cdf ---> Arreglo de frecuencias acumulado
			//mejor no paralelizarlo			
			frecuenciasAcumulado[0]=frecuencias[0];
			for(int i = 1; i < 256;i++){
				if(min_cdf==0){
					min_cdf = frecuenciasAcumulado[i-1];		
				}
				frecuenciasAcumulado[i]=frecuenciasAcumulado[i-1]+frecuencias[i];
			}
		}
		//generamos el nuevo arreglo de frecuencias
		#pragma omp barrier
		#pragma omp for
		for(int i = 0; i < 256; i++){
			frecuenciasAcumuladoEqualizado[i] =round(((((double)frecuenciasAcumulado[i]-min_cdf)/(tamano-min_cdf))*254)+1); 	
		}

		#pragma omp for
		for(int i=0; i < tamano;i++){
			srcImaEqualizada[i] =  frecuenciasAcumuladoEqualizado[srcImaAux[i]];
		}			
	}
	return srcImaEqualizada;	
}

unsigned char* tresCanalesFucionP(unsigned char *src1,unsigned char *src2,unsigned char *src3,int tamano){
	
	unsigned char *src4 = malloc(sizeof(unsigned char) * tamano*3);
	#pragma omp parallel
	{
		#pragma omp for
		for(int i=0;i<tamano;i++){
			src4[i*3]=src1[i];
			src4[i*3+1]=src2[i];
			src4[i*3+2]=src3[i];
		}
	}
	return src4;
}

void ImaParalelo(char *NombreImagen,int channels,int width,int height,char *Imac,unsigned char *srcIma,double tiempoDeCarga){
	char *NombreArchivo = concatenador(Imac,"MetricasParalelo");
	double tiempoDeEjecucion = 0;
	double tiempoDeGeneracion = 0;
	double tiempoDeGeneracionCSV = 0;
	double inicio, final;
	
	if(channels==3){
		inicio = omp_get_wtime();
		int tamano = height*width;
		unsigned char *ima0 = tresCanalesP(srcIma,tamano,0);
		unsigned char *ima1 = tresCanalesP(srcIma,tamano,1);
		unsigned char *ima2 = tresCanalesP(srcIma,tamano,2);
		unsigned char *ima = tresCanalesFucionP(ima0, ima1, ima2,tamano);
		final = omp_get_wtime();
		tiempoDeEjecucion = final- inicio;

		inicio = omp_get_wtime();
		char *Imac1 = concatenador(Imac,"PimerCanalParalelo-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, 1, ima0, 100);

		Imac1 = concatenador(Imac,"SegundoCanalParalelo-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, 1, ima1, 100);

		Imac1 = concatenador(Imac,"TercerCanalParalelo-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, 1, ima2, 100);

		Imac1 = concatenador(Imac,"EcualizacionParalela-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, channels, ima, 100);
		final = omp_get_wtime();		
		tiempoDeGeneracion = final- inicio;

		inicio = omp_get_wtime();
		Imac1 = concatenador(Imac,"Resumen de Histograma Paralelo");		
		generarCSV(Imac1, srcIma, ima, tamano*3);
		final = omp_get_wtime();		
		tiempoDeGeneracionCSV = final- inicio;

		free(Imac1);
		free(ima);
		free(ima0);
		free(ima1);
		free(ima2);

	}else if(channels==1){

		inicio = omp_get_wtime();
		int tamano = height*width;
		unsigned char *ima = unCanalP(srcIma,tamano);
		final = omp_get_wtime();
		tiempoDeEjecucion = final- inicio;

		inicio = omp_get_wtime();
		char *Imac1 = concatenador(Imac,"EcualizacionParalela-");
		Imac1=concatenador(Imac1,NombreImagen);
		stbi_write_jpg(Imac1, width, height, channels, ima, 100);
		final = omp_get_wtime();		
		tiempoDeGeneracion = final- inicio;

		inicio = omp_get_wtime();
		Imac1 = concatenador(Imac,"Resumen de Histograma Paralelo");
		generarCSV(Imac1, srcIma, ima, tamano);
		final = omp_get_wtime();		
		tiempoDeGeneracionCSV = final- inicio;
		
		free(ima);
		free(Imac1);

	}else{
		printf("Numero de canales no contemplado\n");

	}
	generarMetricas(NombreImagen,NombreArchivo,height,width,channels,tiempoDeEjecucion,tiempoDeCarga,tiempoDeGeneracion,tiempoDeGeneracionCSV);	
	
	tiemposMostradosP[0]=tiempoDeEjecucion;
	tiemposMostradosP[1]=tiempoDeCarga;
	tiemposMostradosP[2]=tiempoDeGeneracion;
	tiemposMostradosP[3]=tiempoDeGeneracionCSV;
}