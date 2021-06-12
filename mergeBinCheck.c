#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#define BUFF_SIZE ( 1024 * 1024 )
#define OUTPUT_INFO " " "@ " "\033[0;36m" "0x%08X" "\033[0m" "  Check........... %s ------> %s %s\n"
#define OK "\033[0;32m" "Success" "\033[0m"
#define FAIL "\033[0;31m" "Failure" "\033[0m"
#define RESULT_LOG( ... ) printf( OUTPUT_INFO, addr[i], cmd[ j ], cmd[ CMD_INDEX( file_num, DEST_FILE_CMD_INDEX ) ], __VA_ARGS__ )

bool debug_flag = false;

#define DEBUG_LOG( ... ) \
    if ( debug_flag ) \
    { \
       printf( __VA_ARGS__ ); \
    }

uint8_t buff[BUFF_SIZE];
uint8_t buff2[BUFF_SIZE];

void help( void )
{
    printf("mergeBinCheck.exe help:\n \
            Author:                          LiDeLong\n \
            cmd format:                      mergeBinCheck.exe [offset_addr_1(Hex: 0x...)] [File_1] ... [offset_addr_n(Hex: 0x...)] [File_n] [Target File] [Whether debug log is turned on: 0x01->open, 0x00->close]\n \
            example1(turned on debug log):   mergeBinCheck.exe 0x00001000 RSL.bin 0x00000000 Boot.bin flash.bin 0x01\n \
            example2(turned off debug log):  mergeBinCheck.exe 0x00000000 Boot.bin 0x00001000 RSL.bin flash.bin 0x00\n\n");
}
 
long int get_file_size( FILE *file, bool first_times )
{
    struct stat statbuf;
    long int size = -1;

    if ( first_times )
    {
        size = 0;
    }
    else 
    {
        if ( fseek( file, 0L, SEEK_END ) == 0 )
        {
            size = ftell( file );
        }
    }
    

    return size;
}

int data_cmp( uint8_t *data1, uint8_t *data2, uint32_t size )
{
    int i;
    int ret = -1;

    for ( i = 0; i < size; ++i )
    {
        if ( data1[i] != data2[i] )
        {
            break;
        }
    }

    if ( i == size )
    {
        ret = 0;
    }

    return ret;
}

int data_filling( FILE *file, uint32_t start_addr, uint32_t size, uint8_t data, bool addr_over)
{
    int ret = -1;
    uint32_t need_write_size = 0;
    int set_type;

    if ( addr_over )
    {
        set_type = SEEK_END;
        start_addr = 0;
    }
    else 
    {
        set_type = SEEK_SET;
    }

    memset( buff, data, sizeof( buff ) );

    if ( fseek( file, start_addr, set_type ) == 0 )
    {
        while( size )
        {
            if ( size >= sizeof( buff ) )
            {
                need_write_size = sizeof( buff );
            }
            else 
            {
                need_write_size = size;
            }

            if ( fwrite( ( void * )buff, need_write_size, 1, file ) != 1 )
            {
                ret = -1;
                break;
            }
            size -= need_write_size;
            ret = 0;
        }
    }

    return ret;
}

#define INVALID_NUM ( 3 )
#define DEST_FILE_CMD_INDEX ( 0 )
#define DEST_FILLINT_DATA_CMD_INDEX ( 1 )
#define CMD_INDEX( file_num_, index_ ) ( ( ( file_num_ ) * 2 ) + 1 + ( index_ ) )

int main( int cnt, char *cmd[] )
{
    int ret = 0;
    FILE** file = NULL;
    FILE* dest_file = NULL;
    uint32_t *addr = NULL;
    int i,j;
    int file_num;
    uint32_t src_size, dest_size;
    uint32_t src_end_addr, dest_end_addr, filling_start_addr;
    long int file_size;
    uint32_t temp_size;
    uint8_t filing_data = 0;
    bool first_times = true;

    

    do
    {
        
        if ( cnt <= INVALID_NUM )
        {
            ret = -7;
            help();
            break;
        }

        if ( ( ( cnt - INVALID_NUM ) % 2 ) != 0 )
        {
            ret = -8;
            help();
            break;
        }

        ret = 0;

        file_num = ( ( cnt - INVALID_NUM ) / 2 );

        filing_data = ( uint8_t )strtoul( cmd[ CMD_INDEX( file_num, DEST_FILLINT_DATA_CMD_INDEX ) ], NULL, 16);
        if ( filing_data )
        {
            debug_flag = true;
        }
        else 
        {
            debug_flag = false;
        }

        file = ( FILE ** )malloc( sizeof( FILE* ) * file_num );  
        addr = ( uint32_t * ) malloc( sizeof( uint32_t ) * file_num ); 

        memset( file, 0, sizeof( FILE* ) * file_num ) ;
        memset( addr, 0, sizeof( uint32_t* ) * file_num ) ;

        if ( ( addr == NULL ) || ( file == NULL ) )
        {
            ret = -9;
            break;
        }

        DEBUG_LOG( "file num: %d\n", file_num );

        dest_file = fopen(cmd[ CMD_INDEX( file_num, DEST_FILE_CMD_INDEX ) ], "rb");
        if ( dest_file == NULL )
        {
            DEBUG_LOG(" OPEN file fail: %s\n ", cmd[ CMD_INDEX( file_num, DEST_FILE_CMD_INDEX ) ] );
            ret = -10;
            break;
        }

        for ( i = 0, j = 2; i < file_num; ++i,  j += 2 )
        {
            file[ i ] = fopen(cmd[j], "rb");
            if( file[ i ] == NULL )
            {
                DEBUG_LOG(" OPEN file fail: %s\n ", cmd[ j ]);
                ret = -11;
                break;
            }
        }

        if ( i != file_num )
        {
            break;
        }

        for ( i = 0, j = 1; i < file_num; ++i,  j += 2 )
        {
            addr[i] = ( uint32_t )strtoul( cmd[ j ], NULL, 16);
            DEBUG_LOG ( "offset addr: 0x%08X\n", addr[i] );
        }
        
        for ( i = 0, j = 2; i < file_num; ++i, j += 2 )
        {
            file_size = get_file_size( file[i], false );
            if ( file_size <= 0 )
            {
                DEBUG_LOG( " get file size error: %s\n", cmd[ j ] );
                ret = -12;
                break;
            }
            src_size = ( uint32_t )file_size;
            src_end_addr = src_size - 1;
            DEBUG_LOG( " %s file size: %u\n", cmd[ j ], src_size );

            file_size = get_file_size( dest_file, false );
            if ( file_size <= 0 )
            {
                DEBUG_LOG( " get file size error: %s\n", cmd[ CMD_INDEX( file_num, DEST_FILE_CMD_INDEX ) ] );
                ret = -13;
                break;
            }
            dest_size = ( uint32_t )file_size;
            DEBUG_LOG( " %s file size: %u\n", cmd[ CMD_INDEX( file_num, DEST_FILE_CMD_INDEX ) ], file_size );

            if ( src_size > dest_size )
            {
                RESULT_LOG( FAIL );
                continue;
            }

            if ( addr[i] >= dest_size )
            {
                RESULT_LOG( FAIL );
                continue;
            }

            if ( ( addr[i] + src_size ) > dest_size )
            {
                RESULT_LOG( FAIL );
                continue;
            }

            if ( fseek( file[ i ], 0, SEEK_SET ) != 0 )
            {
                ret = -1;
                break;
            }

            if ( fseek( dest_file, addr[i], SEEK_SET ) != 0 )
            {
                ret = -14;
                break;
            }

            
            while( src_size )
            {

                if ( src_size >= sizeof( buff ) )
                {
                    temp_size = sizeof( buff );
                }
                else 
                {
                    temp_size = src_size;
                }

                memset( ( void * )buff, 0x5A, sizeof( buff ) );
                memset( ( void * )buff2, 0xA5, sizeof( buff ) );

                if ( fread( ( void * )buff, 1, temp_size, file[ i ] ) != temp_size )
                {
                    i = file_num;
                    ret = -2;
                    break;
                }

                if ( fread( ( void * )buff2, 1, temp_size, dest_file ) != temp_size )
                {
                    i = file_num;
                    ret = -15;
                    break;
                }
                src_size -= temp_size;

                if ( data_cmp( buff2, buff, temp_size ) != 0 )
                {
                    src_size = 0;
                    RESULT_LOG( FAIL );
                }
                else 
                {
                    RESULT_LOG( OK );
                }
            }
           
        }
        
    }while(0);





    if ( file )
    {
        for ( i = 0; i < file_num; ++i )
        {
            if ( file[ i ] != NULL )
            {
                fclose( file[ i ] );
            }
        }
    }

    if ( dest_file ) {
        fclose( dest_file );
    }

    if ( file ) {
        free( file );
    }

    if ( addr ) {
        free( addr );
    }

    if ( ret != 0 )
    {
        printf( "\n\nRuning error, error code: %d\n", ret );
    }

    return ret;
}
