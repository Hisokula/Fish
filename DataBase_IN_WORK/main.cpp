#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3/sqlite3.h> 
#include <iostream>
#include <string>
#include <vector>
#include <ctime>

static int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}


enum class TaskType
{
    REGULAR,
    PERSONAL, 
    NONE
};

 // CLASS FOR USER'S TASKS
class Task
{
private:
    //
    time_t now = time(0);
    tm* ltm = localtime(&now);
    //
protected:
    TaskType m_type;

    std::string m_name;
    std::string m_time;
public:
    Task(std::string name) : m_name(name) { std::string m_time = std::to_string(ltm->tm_mday) + ' ' + std::to_string(1 + ltm->tm_mon) + ' ' + std::to_string(1900 + ltm->tm_year); };
    virtual ~Task() = default;

    virtual void SetUp(std::string name)
    {
        m_name = name;
    };

    virtual void ShowInfo()
    {
        std::cout << "Name of task: " << m_name  << std::endl;
    }

    void TimeUpdate()
    {
        m_time = std::to_string(ltm->tm_mday) + ' ' + std::to_string(1 + ltm->tm_mon) + ' ' + std::to_string(1900 + ltm->tm_year);
    }
    virtual std::string GetName()
    {
        return m_name;
    }
};

class PersonalTask : public Task 
{
private:
public:
    PersonalTask(std::string name): Task(name) { m_type = TaskType::PERSONAL;}
    ~PersonalTask() = default;
};


class RegularTask : public Task
{
private:
    int m_frequency;
public:
    RegularTask(std::string name, int freq) : Task(name), m_frequency(freq)
    {m_type = TaskType::REGULAR;};
    ~RegularTask() = default;
    
     void SetUpReg(std::string name, int freq)  
     {
        m_frequency = freq;
        m_name = name;
     }
     virtual void ShowInfo() override
     {
         std::cout << "Name of task: " << m_name << " ,frequency: one time on " << m_frequency << " days." << std::endl;
     }

     void AddToBase(int add_call, sqlite3* database, char* error_mes_flag)
     {
         /* Create SQL statement */
         std::string sql = "INSERT INTO regular_tasks (task,frequency) " \
             "VALUES ('" + m_name + "','" + std::to_string(m_frequency) + "');";
         std::cout << sql << std::endl;

         /* Execute SQL statement */
         add_call = sqlite3_exec(database, sql.c_str(), callback, 0, &error_mes_flag);

         if (add_call != SQLITE_OK) {
             fprintf(stderr, "SQL error: %s\n", error_mes_flag);
             sqlite3_free(error_mes_flag);
         }
         else {
             fprintf(stdout, "Records created successfully\n");
         }
     }
};


// SOME DB FUNK-S



void db_open(sqlite3* db, int rc) /* Open database */
{


    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit;
    }
    else {
        fprintf(stderr, "Opened database successfully\n");
    }
}

void DeleteRegTask(std::string task_name, sqlite3* database)
{
    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM regular_tasks WHERE task=" + task_name;
    const char* del_task = const_cast<char*>(query.c_str());
    sqlite3_prepare_v2(database, del_task, -1, &stmt, NULL);
    sqlite3_step(stmt);
}



int main(int argc, char* argv[]) {
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;

    rc = sqlite3_open("real_my_data.db", &db);

    std::vector<std::string> tasks;
    db_open(db, rc);
    RegularTask task1("Watch_One_Piece", 0);
    RegularTask task2("Watch_JoJo", 0);
    task1.AddToBase(rc, db, zErrMsg);
    task2.AddToBase(rc, db, zErrMsg);

    DeleteRegTask(task1.GetName(),db);
    

    sqlite3_close(db);
    return 0;
}































/*for (int i = 0; i < 1; i++)
    {
        std::string task;
        int frequency;
        std::cin >> task >> frequency;

        */
        /* Create SQL statement */
        /*std::string sql = "INSERT INTO regular_tasks (task,frequency) " \
            "VALUES ('" + task + "','" + std::to_string(frequency) + "');";
        std::cout << sql << std::endl;*/

        /* Execute SQL statement /*
        /*rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "Records created successfully\n");
        }
    }*/
