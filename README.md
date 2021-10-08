Orzata Miruna-Narcisa, 331CA

Tema 3 APD

Cele 5 noduri MPI realizeaza urmatoarele actiuni:
- MASTER -> citeste fisierul de intrare si numara numarul de paragrafe
            din intreg fisierul, dar retine si numarul de paragrafe pentru
            fiecare gen in parte(datele din urma sunt utile nodurilor worker
            pentru a sti cate pachete sa astepte).
        -> porneste 4 thread-uri, care citesc in paralel linie cu linie din
            fisierul de intrare si trimit paragraf cu paragraf la worker-ul
            corespunzator tipului de paragraf de care se ocupa fiecare thread
            in parte.
        -> asteapta textul procesat de la nodurile worker si completeaza un
            dictionar cu id-ul paragrafului ca si cheie si paragraful in sine
            ca valoare.
        -> dupa ce primeste toate paragrafele procesate, nodul MASTER parcurge
            dictionarul in ordinea crescatoare a id-urilor si scrie paragrafele
            procesate in fisierul de iesire.
- WORKER -> porneste un thread care se ocupa de comunicarea cu nodul MASTER
        -> la inceput, nodurile worker primesc de la MASTER numarul total de
            paragrafe de un anumit gen si apoi perechi de mesaje de forma:
            * o structura in care se precizeaza numarul de linii ale paragrafului
                curent si un boolean care imi precizeaza daca paragraful este
                ultimul citit din fisierul de intrare(asta ma ajuta pentru a nu
                mai adauga '\n' la finalul fisierului de iesire, asa cum fac in
                mod normal pentru celelalte paragrafe).
            * un string ce reprezinta paragraful de procesat
        -> thread-ul intermediar deschide alte thread-uri in felul urmator:
            * daca paragraful primit are mai putin de 20 linii, atunci se
                porneste un singur thread;
            * daca paragraful primit are mai mult de 20 linii, atunci e nevoie
                de ceil(nr_linii / 20) thread-uri in total, dar, cum avem la
                dispozitie nr_max_thread-uri, inseamna ca se pornesc mai intai
                nr_max_thread-uri(sau mai putine), se asteapta terminarea executiei
                acestor thread-uri si apoi se pornesc alte thread-uri, se asteapta,
                si asa mai departe pana cand toate bucatile sunt procesate.
        -> dupa ce thread-urile proceseaza bucatile din paragraf, thread-ul intermediar
            care le-a pornit va uni toate partile textului si va crea noul paragraf
            transformat, pe care il va trimite la MASTER. Combinarea tuturor pieselor
            paragrafului a fost posibila datorita unui map care retine id-ul bucatii
            trimisa la thread spre procesare si bucata procesata.  

Discutie legata de scalarea solutiei:
   Din rezultatul checker-ului se poate observa ca solutia implementata
se comporta extrem de bine pe teste foarte mari(testele 4 si 5 dureaza 6
respectiv 8 secunde pe codul secvential, iar pe solutia paralela - 1.8s si 2.5s).
Testele mici par ca ruleaza cu 2 secunde mai mult pe solutia paralela decat
pe cea secventiala, deoarece deschiderea si inchiderea thread-urilor este
o actiune costisitoare pentru CPU si se adauaga acest timp in plus timpului finalul
de executie.