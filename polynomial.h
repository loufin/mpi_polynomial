#ifndef POLYNOMIAL
#define POLYNOMIAL

#define VARIABLE .99
#define COEFFICIENT 1

//double power(double x, int degree);
//double evaluateTerm(int coefficient, int degree, double x);
//void initialize(int coeffArr[], int maxDegree);

//The below are taken from the example code for this assignment
double power(double x, int degree)
{     
      if(degree == 0)  return 1;
      
      if(degree == 1)  return x;

      return x * power(x, degree - 1);
}
double evaluateTerm(int coefficient, int degree, double x)
{
     
      return coefficient * power(x, degree);
}
void initialize(int coeffArr[], int maxDegree)
{
   int i;
   for( i = 0; i < maxDegree; i++)
   {
      coeffArr[i] = COEFFICIENT;
   }
}

#endif
