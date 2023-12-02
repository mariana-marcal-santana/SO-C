#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "files.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

void exeDir(DIR *dir) {
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        // Verifica se o nome do arquivo tem a extensão .jobs
        if (strcmp(entry->d_name + strlen(entry->d_name) - 5, ".jobs") == 0) {
            // Abre o arquivo de entrada
            int fd_input = open(entry->d_name, O_RDONLY);
            if (fd_input == -1) {
                perror("Failed to open input file");
                fprintf(stderr, "File name: %s\n", entry->d_name);
                continue;
            }

            // Cria o nome do arquivo de saída
            char output_filename[strlen(entry->d_name) + 5];
            strcpy(output_filename, entry->d_name);
            strcat(output_filename, ".out");

            // Abre o arquivo de saída
            int fd_output = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd_output == -1) {
                perror("Failed to open output file");
                fprintf(stderr, "File name: %s\n", output_filename);
                close(fd_input);
                continue;
            }

            // Redireciona stdin e stdout
            if (dup2(fd_input, STDIN_FILENO) == -1 || dup2(fd_output, STDOUT_FILENO) == -1) {
                perror("Failed to redirect stdin or stdout");
                close(fd_input);
                close(fd_output);
                continue;
            }

            // Fecha os descritores de arquivo
            close(fd_input);
            close(fd_output);

            // Agora, o stdin e stdout estão redirecionados, e você pode continuar com o restante do seu código
            // ...
        }
    }
}