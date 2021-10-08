#include "mpi.h"
#include <unistd.h>
#include <pthread.h>
#include <bits/stdc++.h>
#define  MASTER 0
#define  HORROR_WORKER 1
#define  COMEDY_WORKER 2
#define  FANTASY_WORKER 3
#define  SF_WORKER 4
#define MASTER_NUM_THREADS 4

using namespace std;

struct info_msg {
    int nr_lines;
    bool last_paragraph;
} __attribute__((__packed__));

MPI_Datatype mpi_msg_info;
unordered_map<int, string> all_paragraphs;
unordered_map<int, string> all_pieces;
pthread_mutex_t my_mutex;


typedef struct master_threads_arguments {
    int thread_id;
    string filename;
    vector<int> nr_paragraphs_by_type;
} master_args;

typedef struct worker_threads_arguments {
    int paragraph_id;
    int worker_type;
    bool last_paragraph;
    int nr_lines;
    string paragraph;
} worker_args;

bool is_consonant(char c){
    if (c < 65 || c > 122 || (c > 90 && c < 97)){
        return false;
    }

    if ((c >= 65 && c <= 90) && c != 'A' && c != 'E' && c != 'I' && c != 'O' && c != 'U'){
        return true;
    }
    if ((c >= 97 && c <= 122) && c != 'a' && c != 'e' && c != 'i' && c != 'o' && c != 'u'){
        return true;
    }
    
    return false;
}

string processHorrorParagraph(string paragraph, int nr_lines, bool last_paragraph){
    string new_paragraph;

    istringstream iss(paragraph);
    string line;
    int i = 1;
    while (getline(iss, line, '\n')) {
        string new_line = "";
        for (char c : line){
            if (is_consonant(c) == true){
                new_line += c;
                new_line += tolower(c);
            } else {
                 new_line += c;
            }
        }
        new_paragraph += new_line;

        if (last_paragraph == true){
            if (i != nr_lines){
                new_paragraph += '\n';
            }
        } else {
            new_paragraph += '\n';
        }
        
        i++;
    }

    return new_paragraph;
}

bool isLetter(char c){
    if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)){
        return true;
    }

    return false;
}

string processComedyParagraph(string paragraph, int nr_lines, bool last_paragraph){
    string new_paragraph;

    istringstream iss(paragraph);
    string line;
    int i = 1;
    while (getline(iss, line, '\n')) {
        string new_line = "";
        istringstream stream2(line);
        string word;
        while (getline(stream2, word, ' ')) {
            int pos = 1;
            for (char c : word){
                if (pos % 2 == 0 && isLetter(c) == true){
                    new_line += toupper(c);
                } else {
                    new_line += c;
                }

                pos++;
            }

            if (stream2.peek() != EOF){
                new_line += ' ';
            }
        }

        new_paragraph += new_line;

        if (last_paragraph == true){
            if (i != nr_lines){
                new_paragraph += '\n';
            }
        } else {
            new_paragraph += '\n';
        }

        i++;
    }

    return new_paragraph;
}

string processFantasyParagraph(string paragraph, int nr_lines, bool last_paragraph){
    string new_paragraph;
    
    istringstream iss(paragraph);
    string line;
    int i = 1;
    while (getline(iss, line, '\n')) {
        string new_line = "";
        istringstream stream2(line);
        string word;
        while (getline(stream2, word, ' ')) {
            int pos = 1;
            for (char c : word){
                if (pos == 1 && isLetter(c) == true){
                    new_line += toupper(c);
                } else {
                    new_line += c;
                }

                pos++;
            }

            if (stream2.peek() != EOF){
                new_line += ' ';
            }
        }

        new_paragraph += new_line;

        if (last_paragraph == true){
            if (i != nr_lines){
                new_paragraph += '\n';
            }
        } else {
            new_paragraph += '\n';
        }

        i++;
    }

    return new_paragraph;
}

string invert(string word){
    string new_word = "";
    for (int i = word.length() - 1; i >= 0; i--){
        new_word += word[i];
    }
    return new_word;
}

string processScienceFictionParagraph(string paragraph, int nr_lines, bool last_paragraph){
    string new_paragraph;

    istringstream iss(paragraph);
    string line;
    int i = 1;
    while (getline(iss, line, '\n')) {
        string new_line = "";
        istringstream stream2(line);
        string word;
        int pos = 1;
        while (getline(stream2, word, ' ')) {
            if (pos == 7){
                new_line += invert(word);
                pos = 0;
            } else {
                new_line += word;
            }
            
            pos++;
            
            if (stream2.peek() != EOF){
                new_line += ' ';
            }
            
        }
        new_paragraph += new_line;

        if (last_paragraph == true){
            if (i != nr_lines){
                new_paragraph += '\n';
            }
        } else {
            new_paragraph += '\n';
        }

        i++;
    }

    return new_paragraph;
}

void sendToWorker(int id_worker, string paragraph, int nr_lines, bool last_paragraph){
    info_msg info;
    info.nr_lines = nr_lines;
    info.last_paragraph = last_paragraph;
    MPI_Send(&info, 1, mpi_msg_info, id_worker, 1, MPI_COMM_WORLD);

    MPI_Send(paragraph.c_str(), paragraph.length(), MPI_CHAR, id_worker, 2, MPI_COMM_WORLD);
}

void sendNumberOfParagraphsToWorkers(int id_worker, int nr_paragraphs){
    MPI_Send(&nr_paragraphs, 1, MPI_INT, id_worker, 0, MPI_COMM_WORLD);
}

string receiveProcessedParagraphFromWorker(int id_worker){
    MPI_Status status;

    MPI_Probe(id_worker, 2, MPI_COMM_WORLD, &status);
    int len;
    MPI_Get_count(&status , MPI_CHAR, &len);
    char *buf = new char[len];
    MPI_Recv(buf, len, MPI_CHAR, id_worker, 2, MPI_COMM_WORLD, &status);

    string new_paragraph(buf, len);
        
    delete[] buf;

    return new_paragraph;
}

void *masterThreadFunction(void *arg) {
    master_args info = *(master_args*) arg;
    int thread_id = info.thread_id;
    string filename = info.filename;
    vector<int> nr_paragraphs_by_type = info.nr_paragraphs_by_type;

    string paragraph_type;
    ifstream myfile;
    myfile.open(filename);

    if(!myfile.is_open()) {
        perror("Error open");
        exit(EXIT_FAILURE);
    }

    int paragraph_id = 0;
    bool last_paragraph = false;

    sendNumberOfParagraphsToWorkers(thread_id + 1, nr_paragraphs_by_type[thread_id]);

    while(getline(myfile, paragraph_type)){
        string paragraph;
        string my_line;
        int nr_lines = 0;
        
        while(getline(myfile, my_line) && my_line.size() != 0){
            paragraph += my_line;
            nr_lines++;
            if (!myfile.eof()){
                paragraph += '\n';
            } else {
                last_paragraph = true;
            }
        }

        if (thread_id == 0 && paragraph_type.compare("horror") == 0){
            sendToWorker(HORROR_WORKER, paragraph, nr_lines, last_paragraph);
            string new_paragraph = receiveProcessedParagraphFromWorker(HORROR_WORKER);
            new_paragraph = "horror\n" + new_paragraph;

            pthread_mutex_lock(&my_mutex);
            all_paragraphs[paragraph_id] = new_paragraph;
            pthread_mutex_unlock(&my_mutex);
            
        } else if (thread_id == 1 && paragraph_type.compare("comedy") == 0){
            sendToWorker(COMEDY_WORKER, paragraph, nr_lines, last_paragraph);
            string new_paragraph = receiveProcessedParagraphFromWorker(COMEDY_WORKER);
            new_paragraph = "comedy\n" + new_paragraph;

            pthread_mutex_lock(&my_mutex);
            all_paragraphs[paragraph_id] = new_paragraph;
            pthread_mutex_unlock(&my_mutex);
            
        } else if (thread_id == 2 && paragraph_type.compare("fantasy") == 0){
            sendToWorker(FANTASY_WORKER, paragraph, nr_lines, last_paragraph);
            string new_paragraph = receiveProcessedParagraphFromWorker(FANTASY_WORKER);
            new_paragraph = "fantasy\n" + new_paragraph;

            pthread_mutex_lock(&my_mutex);
            all_paragraphs[paragraph_id] = new_paragraph;
            pthread_mutex_unlock(&my_mutex);
            
        } else if (thread_id == 3 && paragraph_type.compare("science-fiction") == 0){
            sendToWorker(SF_WORKER, paragraph, nr_lines, last_paragraph);
            string new_paragraph = receiveProcessedParagraphFromWorker(SF_WORKER);
            new_paragraph = "science-fiction\n" + new_paragraph;

            pthread_mutex_lock(&my_mutex);
            all_paragraphs[paragraph_id] = new_paragraph;
            pthread_mutex_unlock(&my_mutex);
        }

        paragraph_id++;
    }

    myfile.close();
    
    pthread_exit(NULL);
}

void *workerThreadFunction(void *arg) {
    worker_args info = *(worker_args*) arg;
    int paragraph_id = info.paragraph_id;
    int worker_type = info.worker_type;
    bool last_paragraph = info.last_paragraph;
    int nr_lines = info.nr_lines;
    string paragraph = info.paragraph;

    string processed_piece;

    if (worker_type == HORROR_WORKER){
        processed_piece = processHorrorParagraph(paragraph, nr_lines, last_paragraph);
        
    } else if (worker_type == COMEDY_WORKER){
        processed_piece = processComedyParagraph(paragraph, nr_lines, last_paragraph);
        
    } else if (worker_type == FANTASY_WORKER){
        processed_piece = processFantasyParagraph(paragraph, nr_lines, last_paragraph);
        
    } else if (worker_type == SF_WORKER){
        processed_piece = processScienceFictionParagraph(paragraph, nr_lines, last_paragraph);
    }

    pthread_mutex_lock(&my_mutex);
    all_pieces[paragraph_id] = processed_piece;
    pthread_mutex_unlock(&my_mutex);

    pthread_exit(NULL);
}

string startWorkerThreadsToProcessText(int worker_type, int nr_max_threads, string paragraph, int nr_lines, bool last_paragraph){
    string new_paragraph;

    int processing_steps = ceil((double)nr_lines / 20);
    int nr_remaining_lines = nr_lines;
    int paragraph_id = 0;
    istringstream stream(paragraph);

    while(processing_steps != 0){

        pthread_t threads[nr_max_threads];
        int r;
        int id;
        void *status;
        worker_args my_args[nr_max_threads];
        int active_threads = 0;

        for (id = 0; id < nr_max_threads; id++) {
            my_args[id].paragraph_id = paragraph_id;
            my_args[id].worker_type = worker_type;

            int dif = nr_remaining_lines - 20;
            if (dif > 0){
                my_args[id].nr_lines = 20;
                nr_remaining_lines = dif;
                my_args[id].last_paragraph = false;
            } else {
                my_args[id].nr_lines = nr_remaining_lines;
                nr_remaining_lines = 0;
                if (last_paragraph == true){
                    my_args[id].last_paragraph = true;
                } else {
                    my_args[id].last_paragraph = false;
                }
            }

            string line;
            string piece_for_processing = "";
            for (int i = 0; i < my_args[id].nr_lines; i++){
                getline(stream, line, '\n');
                piece_for_processing += line;
                if (my_args[id].last_paragraph == false || stream.peek() != EOF){
                    piece_for_processing += '\n';
                }
            }
            
            my_args[id].paragraph = piece_for_processing;
            paragraph_id++;
            r = pthread_create(&threads[id], NULL, workerThreadFunction, (void *) &my_args[id]);
    
            if (r) {
                printf("Eroare la crearea thread-ului %d\n", id);
                exit(-1);
            }

            active_threads++;
            processing_steps--;
            if (processing_steps == 0){
                break;
            }
        }
    
        for (id = 0; id < active_threads; id++) {
            r = pthread_join(threads[id], &status);
    
            if (r) {
                printf("Eroare la asteptarea thread-ului %d\n", id);
                exit(-1);
            }
        }
    }

    for (int i = 0; i < paragraph_id; i++){
        new_paragraph += all_pieces.at(i);
    }
    
    return new_paragraph;
}

void *IntermediaryWorkerThreadFunction(void *arg) {
    int worker_type = *(int*) arg;

    int nr_max_threads = sysconf(_SC_NPROCESSORS_CONF) - 1;

    int nr_paragraphs;
    MPI_Status status;
    
    MPI_Recv(&nr_paragraphs, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);

    for(int i = 0; i < nr_paragraphs; i++){
        struct info_msg msg;
        MPI_Recv(&msg, 1, mpi_msg_info, MASTER, 1, MPI_COMM_WORLD, &status);

        MPI_Probe(MASTER, 2, MPI_COMM_WORLD, &status);
        int len;
        MPI_Get_count(&status , MPI_CHAR, &len);
        char *buf = new char[len];
        MPI_Recv(buf, len, MPI_CHAR, MASTER, 2, MPI_COMM_WORLD, &status);

        string paragraph(buf, len);
        
        delete[] buf;

        string new_paragraph = startWorkerThreadsToProcessText(worker_type, nr_max_threads, paragraph, msg.nr_lines, msg.last_paragraph);
        MPI_Send(new_paragraph.c_str(), new_paragraph.length(), MPI_CHAR, MASTER, 2, MPI_COMM_WORLD);
    }

    pthread_exit(NULL);
}

void startIntermediaryWorkerThread(int worker_type){
    pthread_t intermediary_thread;
    int r;
    void *status;

    r = pthread_create(&intermediary_thread, NULL, IntermediaryWorkerThreadFunction, (void *) &worker_type);
 
    if (r) {
        printf("Eroare la crearea thread-ului intermediar\n");
        exit(-1);
    }
 
    r = pthread_join(intermediary_thread, &status);
 
    if (r) {
        printf("Eroare la asteptarea thread-ului intermediar\n");
        exit(-1);
    }
}

void get_args(int argc, char *argv[], string* filename){
    if (argc < 2) {
		cout << "Numar insuficient de parametri:\n\t";
        cout << "mpirun -np 5 tema3 some_file.in\n";
		exit(EXIT_FAILURE);
	}
    if (argc == 2){
        *filename = argv[1];
    }
}

vector<int> readFile(string filename, int &nr_paragraphs){
    vector<int> nr_paragraphs_by_type(4, 0);

    string paragraph_type;
    ifstream myfile;
    myfile.open(filename);

    if(!myfile.is_open()) {
        perror("Error open");
        exit(EXIT_FAILURE);
    }


    while(getline(myfile, paragraph_type)){
        string my_line;

        while(getline(myfile, my_line) && my_line.size() != 0){
            // just read the paragraph
        }

        if (paragraph_type.compare("horror") == 0){
            nr_paragraphs_by_type[0]++;
            
        } else if (paragraph_type.compare("comedy") == 0){
            nr_paragraphs_by_type[1]++;
            
        } else if (paragraph_type.compare("fantasy") == 0){
            nr_paragraphs_by_type[2]++;
            
        } else if (paragraph_type.compare("science-fiction") == 0){
            nr_paragraphs_by_type[3]++;
        }
        nr_paragraphs++;
    }

    myfile.close();

    return nr_paragraphs_by_type;
}

void writeAllParagraphsInFile(string input_filename, int nr_paragraphs){
    string output_filename = ".";
    istringstream iss(input_filename);
    string name;
    if (getline(iss, name, '.')){
        output_filename += name; 
    } else {
        printf("Error\n");
        exit(-1);
    }

    if (getline(iss, name, '.')){
        output_filename += name + ".out"; 
    } else {
        printf("Error\n");
        exit(-1);
    }

    std::ofstream out(output_filename);

    for (int i = 0; i < nr_paragraphs; i++){
        out << all_paragraphs.at(i);
        if (i != nr_paragraphs - 1){
            out << '\n';
        }
    }

    out.close();
}

int main (int argc, char *argv[])
{
    int numtasks, rank, len;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    string filename;
    get_args(argc, argv, &filename);

    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks); // Total number of processes.
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); // The current process ID / Rank.
    MPI_Get_processor_name(hostname, &len);

    // new type
    const int num_items = 2;
    int blocklen[2] = {1, 1};
    MPI_Aint offsets[2];

    offsets[0] = offsetof(struct info_msg, nr_lines);
    offsets[1] = offsetof(struct info_msg, last_paragraph);

    MPI_Datatype types[2] = {MPI_INT, MPI_CXX_BOOL};

    MPI_Type_create_struct(num_items, blocklen, offsets, types, &mpi_msg_info);
    MPI_Type_commit(&mpi_msg_info);


    if (rank == MASTER) {
        int nr_paragraphs = 0;
        vector<int> nr_paragraphs_by_type = readFile(filename, nr_paragraphs);

        pthread_t threads[MASTER_NUM_THREADS];
        int r;
        int id;
        void *status;
        master_args my_args[MASTER_NUM_THREADS];

        r = pthread_mutex_init(&my_mutex, NULL);
        if (r) {
            printf("Eroare la initializarea mutex-ului\n");
		    exit(-1);
	    }

        for (id = 0; id < MASTER_NUM_THREADS; id++) {
            my_args[id].thread_id = id;
            my_args[id].filename = filename;
            my_args[id].nr_paragraphs_by_type = nr_paragraphs_by_type;
        }

        for (id = 0; id < MASTER_NUM_THREADS; id++) {
            r = pthread_create(&threads[id], NULL, masterThreadFunction, (void *) &my_args[id]);
 
            if (r) {
                printf("Eroare la crearea thread-ului %d\n", id);
                exit(-1);
            }
        }
 
        for (id = 0; id < MASTER_NUM_THREADS; id++) {
            r = pthread_join(threads[id], &status);
 
            if (r) {
                printf("Eroare la asteptarea thread-ului %d\n", id);
                exit(-1);
            }
        }


        r = pthread_mutex_destroy(&my_mutex);
        if (r) {
            printf("Eroare la distrugerea mutex-ului\n");
            exit(-1);
        }

        writeAllParagraphsInFile(filename, nr_paragraphs);

    } else if (rank == HORROR_WORKER){
        startIntermediaryWorkerThread(HORROR_WORKER);

    } else if (rank == COMEDY_WORKER){
        startIntermediaryWorkerThread(COMEDY_WORKER);
        
    } else if (rank == FANTASY_WORKER){
        startIntermediaryWorkerThread(FANTASY_WORKER);

    } else if (rank == SF_WORKER){
        startIntermediaryWorkerThread(SF_WORKER);
    }
    
    MPI_Type_free(&mpi_msg_info);
    MPI_Finalize();

}
