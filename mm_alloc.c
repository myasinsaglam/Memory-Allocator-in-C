//
// Created by M.Yasin SAĞLAM on 23.12.2017.
//

/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include "mm_alloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>

pthread_mutex_t lock; //thread safe olmasi icin 1 adet global lock kullanmak yeterlidir

struct block *head,*tail; //linkli listenin basi ve sonu

typedef long unsigned int addrtype;
void mm_print_mem() {

    // linked list head pointer MEM_ALLOC_ROOT
    printf( "\nstart_addr\tsize\tfree\tprev\tnext\n");
    printf("=============================\n");

    // check if first block is free

    printf("HEAD OF LL %ld\n", (addrtype)head);
    struct block *iter=head;
    struct block * currentPtr = iter;
    int i = 0;
    while (currentPtr!= NULL  && i <= 10) {
        printf("%ld\t%ld\t%ld\t%ld\t%ld\n", (addrtype) currentPtr, (addrtype)currentPtr->size,
                (addrtype)currentPtr->is_free,(addrtype)currentPtr->prev,(addrtype)currentPtr->next);
        if (currentPtr->next == NULL) break;
        currentPtr = currentPtr->next;
        i++;
    }
}

//bos olan ilk blogun adresini donduren fonksiyon
//first fit allocation yapilmistir
struct block* find_free_block(size_t size){
    struct block *iter=head; //headi kaybetmemek icin itere atar gezeriz
    while (iter){ //daha once allocation yapilmissa
        if(iter->is_free && iter->size>=size) //eger bossa ve boyut olarak yeterliyse
            return iter; //blogu dondur
        iter=iter->next;// sart uygun degilse ilerle
    }
    return NULL; //uygun bos yer yok heapte yeni alan acilacak bu yuzden null dondur
}

void *mm_malloc(size_t size) {
    struct block* here; //mevcut yeri tutan pointer
    size_t total_size;
    void* area; //bos olan alan adresi void ptr tipinde
    if(!size){
        printf("\nSize error Size must be greater than 0");
        return NULL;
    }
    mm_print_mem();//yazdirma fonksiyonu
    //allocation baslasin
    pthread_mutex_lock(&lock);//thread safe olmasi icin lock aktif ediliyor
    here=find_free_block(size); //istenilen alan kadar ilk bos olan block araniyor
    if(here){ //eger bulunduysa
        here->is_free=0; //dolu yapiliyor
        pthread_mutex_unlock(&lock);//cikmadan once lock kalkiyor
        return (void*)(here+1); //header bilgisini ezmemek icin 1 sonraki adres yani datanin yerlesecegi adres donduruluyor
    }
    //eger null donduyse memory genisletilecek
    total_size=BLOCK_SIZE+size; //toplam boyut struct boyutu+istenen alan kadar
    area=sbrk(total_size); //heapi genislet
    if(area==(void*)-1){ //eger genisleme olmamissa donen degeri kontrol et
        printf("\nHeap Yetersiz"); //mesaj yazdir
        pthread_mutex_unlock(&lock);//cikmadan once lock kalkiyor
        return NULL; //NULL dondur
    }
    //basarili bir sekilde allocate edilmisse
    here=area; //mevcut yeri tutan pointer adresini heap area adresi yap
    here->is_free=0; //block dolu
    here->size=size; //block data boyutunu ata
    memset((void*)(here+1),0,size); //data blogu soruda istenildigi gibi sifirla dolduruluyor
    here->next=NULL; //sonrasinda bir blok gelmemistir sbrk ile heapi genislettik cunku
    if(!head){ //eger ilk kez yapiyorsak head degisir linli liste mantigi
        head=here;
    }
    if(!tail){ //tail de ayni sekilde
        tail=here;
    }
    else{ //eger oncesinde bir block varsa listenin sonuna ekleriz
        tail->next=here; //onceki blogun nexti olacak
        here->prev=tail; //ekledigimiz de onceki blogu tutacak
        tail=here; //son eklenen blok tail yapiliyor
    }
    pthread_mutex_unlock(&lock);//cikmadan once lock kalkiyor
    return (void*)(here+1); //head bilgisi atlanarak data yerlestirilecek blogun adresi donduruluyor
}

//burada ilgili blogu tasiyabilecegimiz bir yer var mi ona bakacagiz
//yoksa malloc yapip eski blogu kopyaladiktan sonra eski blogu free yaparak bir cozum bulmus oluruz
void *mm_realloc(void *ptr, size_t size) {
    struct block* header;
    void* bigger;
    if(!ptr || size==0) //eger boyut sifirsa veya dizi yoksa malloc fonk gecis yapilir
        return malloc(size); //malloc zaten threadsafe burada gerek yok
    //burada threadsafe e gerek var
    pthread_mutex_lock(&lock); //lock koyulur memory erisilecek
    header=(struct block*)ptr-1; //ilgili blogun header i cekiliyor
    if(header->size==size){//ayni boyuta realloc yapmak istiyorsa degisiklige gerek yok
        pthread_mutex_unlock(&lock);
        return ptr;
    }
    else if(header->size>size){//dizi kucultulmek isteniyorsa
        header->size=size; //header daki size degistirilir
        pthread_mutex_unlock(&lock);//lock kalkar
        return ptr;
    }
    else if(header->size<size){//dizi(data blogu) buyutulmek isteniyorsa
        pthread_mutex_unlock(&lock);//lock kalkar malloc calissin diye
        bigger=malloc(size); //daha buyuk bir yer ayrilir
        pthread_mutex_lock(&lock);//lock tekrar koyulur
        if(bigger){//eger basarili olmussa
            //yeni ayrilan alana eski alandaki bilgiler kopyalanir
            memcpy(bigger,ptr,header->size);//eski alandaki boyut kadar data blogu yeni data bloguna kopyalanir
            pthread_mutex_unlock(&lock);//lock kalkar free calissin diye
            free(ptr);//eski block free yapilir
            return bigger; //daha buyuk data blogu adresi dondurulur
        }
    }
    pthread_mutex_unlock(&lock);//lock kalkar cikilir
    return NULL;
}

void mm_free(void *ptr) {
    struct block *header;
    void *end_heap;//heapin neresindeyiz en son dolu oldugu yer neresi
    size_t chunk;//sacma anlamsiz boslugu tutar heap in son kaldigi yer ile son blogun data blogundan sonraki kisim
    if(!ptr)  //eger verilen adreste bisey yoksa
        return; //cikis
    //varsa free yapacagiz
    pthread_mutex_lock(&lock);//threadsafe icin lock aktif olur
    //header bilgisinin adresi ataniyor
    header=(struct block*)ptr-1; //data adresinin 1 byte gerisinde ilgili header bulunuyor
	printf("/nFree yapiyoruz adres %ld",(addrtype)header);
    end_heap=sbrk(0);//sbrk(0) bize heapin mevcut kalinan en son yerini verecektir.
    //simdi ilgili block en sonda ise heapi kucultmeliyiz degilse blogun isfree degiskenini 1 yapmaliyiz
    //boylelikle tekrardan oraya baska bilgi yazilabilir.
    //blogun degerlerinin en sona yerlestigini anlamak icin
    //baslangic arti data ile blogun en son nerede bittigini buluruz
    if(header->next==NULL){//eger o blogun sonrasi null ise son block demektir.
        if(head==tail){//eger 1 tane block kalmissa
            head=tail=NULL; //linkli liste baslangic durumunu alir
        }
        else{//son blogun adresi linkli listeden cikarilir
            tail=tail->prev;//yani tail bir onceki eleman olur ve nexti null olur
            tail->next=NULL;
        }
        //sonrasinda ise sbrk ile heap kucultulur
        //kucultme miktari ise elimizdeki artik alan+header boyutu+header da tutulan block size kadar olur
        chunk=end_heap-(ptr+header->size); //artik alan demektir
        sbrk(0- BLOCK_SIZE-header->size-chunk);
        //locku kaldirir ve return ederiz
        pthread_mutex_unlock(&lock);
        return;
    }
    //eger free yapilmak istenen block son block degilse
    //sadece header yapisindaki isfree degiskenini 1 yapariz
    header->is_free=1;
    pthread_mutex_unlock(&lock);//locku kaldiririz
}

