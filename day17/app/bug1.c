void api_putch(int c);
void api_putstr(char *ch);

int main(){
    for(;;){
        api_putstr("this is test");
    }

    return 0;
}
