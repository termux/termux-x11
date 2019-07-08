#if defined(bit64)
	#define Elf_Ehdr Elf64_Ehdr
	#define Elf_Shdr Elf64_Shdr
	#define Elf_Dyn Elf64_Dyn
	#define dlneeds dlneeds64
#else
	#define Elf_Ehdr Elf32_Ehdr
	#define Elf_Shdr Elf32_Shdr
	#define Elf_Dyn Elf32_Dyn
	#define dlneeds dlneeds32
#endif

char ** dlneeds(int fd)
{
    Elf_Ehdr hdr;
    char *strdata = 0;
    char **result = NULL;
    int i;

    if( read( fd, &hdr, sizeof( Elf_Ehdr ) ) < 0 ) {
        LOGE("Cannot read elf header: %s", strerror( errno ) );
        return NULL;
    }

    /* Jump to the section header table. */
    lseek( fd, hdr.e_shoff, SEEK_SET );

    /* Find and load the dynamic symbol strings. */
    for( i = 0; i < hdr.e_shnum; i++ ) {
        Elf_Shdr shdr;

        lseek( fd, hdr.e_shoff + (i * hdr.e_shentsize), SEEK_SET );
        read( fd, &shdr, sizeof( Elf_Shdr ) );

        if( shdr.sh_type == SHT_DYNSYM ) {
            Elf_Shdr symstr;

            lseek( fd, hdr.e_shoff + (shdr.sh_link * hdr.e_shentsize),
                   SEEK_SET );
            read( fd, &symstr, sizeof( Elf_Shdr ) );
            lseek( fd, symstr.sh_offset, SEEK_SET );
            strdata = malloc( symstr.sh_size );
            read( fd, strdata, symstr.sh_size );
            break;
        }
    }

    /* Quit if we couldn't find any. */
    if( strdata == 0 ) {
        LOGE("No symbol strings found.");
        close(fd);
        return NULL;
    }

    /* Now find the .dynamic section. */
    for( i = 0; i < hdr.e_shnum; i++ ) {
        Elf_Shdr shdr;

        lseek( fd, hdr.e_shoff + (i * hdr.e_shentsize), SEEK_SET );
        read( fd, &shdr, sizeof( Elf_Shdr ) );

        if( shdr.sh_type == SHT_DYNAMIC ) {
            int j;
            int k;
            int needed_count = 0;
            Elf_Dyn cur;

            /* Load its data and print all DT_NEEDED strings. */
            lseek( fd, shdr.sh_offset, SEEK_SET );
            for(j=0; j<shdr.sh_size/sizeof(Elf_Dyn); j++) {
                read( fd, &cur, sizeof(Elf_Dyn));
                if( cur.d_tag == DT_NEEDED ) needed_count++;
            }
            
            result = calloc(1, (needed_count + 1) * sizeof(char*));
            if (result == NULL) {
				LOGE("Failed to allocate dlneeds result");
				close(fd);
				return NULL;
			}
			
            lseek( fd, shdr.sh_offset, SEEK_SET );
            for( j = 0, k = 0; j < shdr.sh_size / sizeof(Elf_Dyn); j++ ) {
                read( fd, &cur, sizeof( Elf_Dyn ) );
                if( cur.d_tag == DT_NEEDED ) {
                    result[k] = strdup(strdata + cur.d_un.d_val);
                    k++;
                }
            }
        }
    }

	return result;
}

#undef Elf_Ehdr
#undef Elf_Shdr
#undef Elf_Dyn
#undef dlneeds
