
void functionOne()
{
}
void functionTwo()
{
}
int main()
{
   for (int i = 0; i < 100; ++i)
   {
      if (i % 3 == 0)
         functionOne();
      else
         functionTwo();
   }
   return 0;
}
