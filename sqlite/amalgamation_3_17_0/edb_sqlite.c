#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "sqlite3.h"

#define edb_sqlite      "/3rd_rw/edb.db"
/* 
 * Reference document https://www.sqlite.org
 *
 * # Table define
 * CREATE TABLE t_edb(channelId INT, eventId INT,eventDetail TEXT);
 * CREATE INDEX index_ch_event on t_edb(channelId,eventId)
 *
 */


#define BEGIN_TIMER beginTimer()
#define END_TIMER   endTimer()

static  sqlite3 *g_pt_edb   = NULL;
static  int     startIndex  = 0;    


/*
 ** Open edb sqlite database
 */
int openDB(void)
{
    int    rc = 0;
    //open database by file name
    rc = sqlite3_open(edb_sqlite, &g_pt_edb);
    if(rc)
    {
        printf("Can't oen database: %s\n",edb_sqlite);
        return -1;
    }

    printf("Open database: %s successful\n",edb_sqlite);
    return 0;
}

/*
 ** Close edb sqlite database
 */
int closeDB(void)
{
    int    rc = 0;
    //close database 
    if(g_pt_edb!=NULL)
    {
        sqlite3_close(g_pt_edb);
        g_pt_edb = NULL;
    }
    return 0;
}


/*
 ** Show current total memory
 */
static void showMemoryUsed(void)
{
    int iCur;
    int iHiwtr;

    iHiwtr = iCur = -1;
    sqlite3_status(SQLITE_STATUS_MEMORY_USED, &iCur, &iHiwtr, 0);//iCur is current total memory
    printf("%d\n", iCur);
}


static struct rusage sBegin;

/*
 ** Begin timing an operation
 */
static void beginTimer(void){
    getrusage(RUSAGE_SELF, &sBegin);
}

/* Return the difference of two time_structs in seconds */
static double timeDiff(struct timeval *pStart, struct timeval *pEnd){
    return (pEnd->tv_usec - pStart->tv_usec)*0.000001 + 
        (double)(pEnd->tv_sec - pStart->tv_sec);
}

/*
 ** Print the timing results.
 */
static void endTimer(void){
    struct rusage sEnd;
    getrusage(RUSAGE_SELF, &sEnd);
    printf("%f\t%f\t",
           timeDiff(&sBegin.ru_utime, &sEnd.ru_utime),
           timeDiff(&sBegin.ru_stime, &sEnd.ru_stime));

    showMemoryUsed();
}

/*
 ** Insert 1 row to database.
 */
static int insertRow(void)
{
    int     i               = 0;
    int     rc              = 0;
    char    testString[500] = "\0";
    char    insertSql[]     = "INSERT INTO t_edb VALUES(@channelId,@eventId,@eventDetail)";
    char    v = 0x41;
    sqlite3_stmt    *stmt   = NULL;
    const char*     zTail   = NULL;
    int             testCount = 1;



    BEGIN_TIMER;
    rc = sqlite3_prepare_v2(g_pt_edb,insertSql,sizeof(insertSql),&stmt,&zTail);
    if(rc)
    {
        printf("sqlite3_prepare fail,err=%d\n",rc);
        return -1;
    }

    for(i=startIndex; i<startIndex + testCount; i++)
    {
        v++;
        if(v>=0x7A) v= 0x41;
        memset(testString,v,sizeof(testString));//generator ramdom ascii string 
        testString[499]=0x0;
        sqlite3_bind_int(stmt,1,i);
        sqlite3_bind_int(stmt,2,i);
        sqlite3_bind_text(stmt,3,testString,-1,SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            printf("Data bind fail\n");
        }
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
    }
    startIndex += testCount;

    sqlite3_finalize(stmt);
    END_TIMER;


    return rc;
}


/*
 ** Batch Insert 1000 rows into database, 500+ bytes per row.
 */
static int insertRows(int count)
{
    int     i               = 0;
    int     rc              = 0;
    char    testString[500] = "\0";
    char    insertSql[]     = "INSERT INTO t_edb VALUES(@channelId,@eventId,@eventDetail)";
    char    v = 0x41;
    sqlite3_stmt    *stmt   = NULL;
    const char*     zTail   = NULL;


    //Insert 1000 rows into database, 500+ bytes per row
    BEGIN_TIMER;
    rc = sqlite3_prepare_v2(g_pt_edb,insertSql,sizeof(insertSql),&stmt,&zTail);
    if(rc)
    {
        printf("sqlite3_prepare fail,err=%d\n",rc);
        return -1;
    }

    //using transaction increase insert speed
    sqlite3_exec(g_pt_edb,"BEGIN TRANSACTION",NULL,NULL,NULL);

    for(i=startIndex; i<startIndex + count; i++)
    {
        v++; if(v>=0x7A) v= 0x41;
        memset(testString,v,sizeof(testString));
        testString[499]=0x0; //Add \0
        sqlite3_bind_int (stmt,1,i);
        sqlite3_bind_int (stmt,2,i);
        sqlite3_bind_text(stmt,3,testString,-1,SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            printf("Data bind fail\n");
        }
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
    }
    startIndex += count;

    rc = sqlite3_exec(g_pt_edb,"END TRANSACTION",NULL,NULL,NULL);
    sqlite3_finalize(stmt);
    END_TIMER;

    return rc;
}

static int testNo = 2;//For random get row by channelId & eventId


/*
 ** Test select 1 row from database by random channelId & eventid.
 */
int testSelect(void)
{
    int     rc              = 0;
    char    selectSql[]     = "SELECT eventDetail from t_edb where channelId=? and eventid=?";
    sqlite3_stmt    *stmt   = NULL;
    const char*     zTail   = NULL;
    int             step    = 0;

    BEGIN_TIMER;
    rc = sqlite3_prepare_v2(g_pt_edb,selectSql,sizeof(selectSql),&stmt,&zTail);
    if(rc)
    {
        printf("sqlite3_prepare fail,err=%d\n",rc);
        return -1;
    }

    //random select by channelId,EventId
    sqlite3_bind_int(stmt,1,startIndex/testNo);//Para 1
    sqlite3_bind_int(stmt,2,startIndex/testNo);//Para 2

    rc = sqlite3_step(stmt);
    if(rc == SQLITE_ROW)
    {
        //Get event detail text buffer by  "sqlite3_column_text(stmt,0);"
    }else{
        printf("Select data error %d ",startIndex/testNo);
    }

    sqlite3_finalize(stmt);
    END_TIMER;

    testNo++;
    return rc;
}

/*
 ** Test delete 1 row from database by random channelId & eventid.
 */
int testDelete(void)
{
    int     rc              = 0;
    char    selectSql[]     = "DELETE from t_edb where channelId=? and eventid=?";
    sqlite3_stmt    *stmt   = NULL;
    const char*     zTail   = NULL;
    int             step    = 0;

    BEGIN_TIMER;
    rc = sqlite3_prepare_v2(g_pt_edb,selectSql,sizeof(selectSql),&stmt,&zTail);
    if(rc)
    {
        printf("sqlite3_prepare fail,err=%d\n",rc);
        return -1;
    }

    //random select by channelId,EventId
    sqlite3_bind_int(stmt,1,startIndex/testNo);//Para 1
    sqlite3_bind_int(stmt,2,startIndex/testNo);//Para 2

    rc = sqlite3_step(stmt);
    if(rc == SQLITE_DONE)
    {
        //Get event detail text buffer by  "sqlite3_column_text(stmt,0);"
    }else{
        printf("Delete data error %d ret=%d",startIndex/testNo,rc);
    }

    sqlite3_finalize(stmt);
    END_TIMER;

    testNo++;
    return rc;
}


int main(int argc,char** args)
{
    int             i           = 0;
    int             rc          = 0;
    long            limit_size  = 0;
    long            insert_row_number = 0;

    if(argc < 3)
    {
        printf("./test limit_memory_size(bytes) InsertRowNumber\n");
        return -1;
    }

    limit_size = strtol(args[1],NULL,10);
    if(limit_size < 1*1024*1024)
    {
        limit_size = 1*1024*1024;//Default 1M
    }

    insert_row_number = strtol(args[2],NULL,10);
    if(insert_row_number < 500)
    {
        insert_row_number = 500;//Default 1M
    }



    printf("Limit memory size = %ld insertRows=%ld\n",limit_size,insert_row_number);

    sqlite3_soft_heap_limit(limit_size);//default limit 1M memory
    rc = openDB(); 
    if(rc != 0)
    {
        printf("Can't oen database: edb.db\n");
        return -1;
    }
    showMemoryUsed();


    printf("Insert 50 times * %d events to database\n",insert_row_number);
    printf("User time\tSys time\tMemory used\n");
    printf("===========================================\n");
    for(i=0; i<50; i++)
    {
        insertRows(insert_row_number);
    }
    printf("===========================================\n\n");


    printf("Insert 50 times * 1 event to database\n");
    printf("User time\tSys time\tMemory used\n");
    printf("===========================================\n");
    for(i=0; i<50; i++)
    {
        insertRow();
    }
    printf("===========================================\n\n");


    printf("Select 1 event from database\n");
    printf("User time\tSys time\tMemory used\n");
    printf("===========================================\n");
    for(i=0; i<50; i++)
    {
        testSelect();
    }
    printf("===========================================\n\n");

    testNo = 2;//reset random index
    printf("Delete 1 event from database\n");
    printf("User time\tSys time\tMemory used\n");
    printf("===========================================\n");
    for(i=0; i<50; i++)
    {
        testDelete();
    }
    printf("===========================================\n\n");



    closeDB();
    return 0;
}
