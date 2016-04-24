int main()
{
    char method_arr[MAXDATASIZE];
    char unparsed_url_arr[MAXDATASIZE];
    char http_arr[MAXDATASIZE];
    char header_arr[MAXDATASIZE]
    char hostname_arr[MAXDATASIZE];
    char path_arr[MAXDATASIZE];
    int method_len = 0;
    int url_len = 0;
    int http_len = 0;
    int header_len = 0;
    int hostname_len = 0;
    int path_len = 0;
    char* reader_loc = method_arr;
    int* counter_ptr = &method_len;
    bool seen_whitespace = false;
    int num_slashes = 0;
    int hostname_index;

    if(argc > 3) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    for(unsigned int i = 0; i < strlen(argv[1]); i++){
        if(argv[1][i] == ' ' && seen_whitespace == false){
            seen_whitespace = true;
            reader_loc = unparsed_url_arr;
            counter_ptr = &unparsed_url_len;
            continue;

        }
        if(argv[1][i] == ' ' && seen_whitespace == true){
            reader_loc = http_arr;
            counter_ptr = &http_len;
            continue;
        }

        if(argv[1][i] == '\n'){
            reader_loc = header_arr;
            counter_ptr = header_len;
            continue;
        }

        *reader_loc = argv[1][i];
        reader_loc++;
        *counter_ptr += 1;
    }
    *reader_loc = '\0';

    for(unsigned int i = 0; i < strlen(unparsed_url_arr); i++){
        if(unparsed_url_arr[i] == '/')
            num_slashes++;
        if(num_slashes == 2){
            hostname_index = i+1;
            reader_loc = hostname_arr;
            counter_ptr = hostname_len;
            break;
        }

    }

    for(unsigned int i = hostname_index; i < strlen(unparsed_url_arr); i++){
        if(unparsed_url_arr[i] == '/'){
            reader_loc = path_arr;
            counter_ptr = path_len;
        }

        *reader_loc = unparsed_url_arr[i];
        reader_loc++;
        *counter_ptr += 1;
    }







}