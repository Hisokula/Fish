#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3/sqlite3.h> 
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>

//векторы, которые нужны
std::vector<int> tasks_reg_freq; //частота, связан с tasks_reg_all
std::vector<std::string> tasks_reg_null; //todays buisness
std::vector<std::string> tasks_reg_all; //все regular таски из bd(без фильтра) [ТРЕБУЕТ ОБЯЗАТЕЛЬНОГО ВЫЗОВА В МЕЙНЕ ОТ -arg в FiltAndFill, иначе пустой]
std::vector<std::string> tasks_pers;//все персональные таски из bd

//5 cumбеков
//1) Стандартный(для делита). Ничего не печатает, никуда ничего не пушит
static int callback(void* NotUsed, int argc, char** argv, char** azColName) 
{
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

//2) Непонятная херня с форума для tab_set_up
static int callback_2(void* pString, int argc, char** argv, char** azColName) {
    if (argc > 0) {
        std::string* str = static_cast<std::string*>(pString);
        str->assign(argv[0]);
    }
    return 0;
}

//3) Заполняет фильтрованное для txt(&more)
static int callback_3(void* pString, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i ++)
    {
        if (i % 2 == 0)
        {
            tasks_reg_null.push_back(argv[i]);
        }

    }
    return 0;
}

//4) Заполняет персоналы
static int callback_4(void* pString, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++)
    {
        tasks_pers.push_back(argv[i]);
    }
    return 0;
}

//5) Работает. Заполняет все регуляры и их фреки
static int callback_5(void* pString, int argc, char** argv, char** azColName) 
{
    for (int i = 0; i < argc; i++)
    {
        if (i % 2 == 0)
        {
            tasks_reg_all.push_back(argv[i]);
        }
        else
        {
            tasks_reg_freq.push_back(static_cast<int>(*argv[i]));
        }

    }
    return 0;
}


enum class TaskType
{
    REGULAR,
    PERSONAL
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
        std::cout << "Name of task: " << m_name << std::endl;
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

class PersonalTask : public Task //______________________________________________________________________ ХХХсделать дефолтный конструкторХХХ ______________________________________________________________________
{
private:
public:
    PersonalTask(std::string name) : Task(name) { m_type = TaskType::PERSONAL; }

    ~PersonalTask() = default;

    void AddToBase(int add_call, sqlite3* database, char* error_mes_flag)
    {
        /* Create SQL statement */
        std::string sql = "INSERT INTO personal_tasks (task) " \
            "VALUES ('" + m_name + "');";
        std::cout << sql << std::endl;

        /* Execute SQL statement */
        add_call = sqlite3_exec(database, sql.c_str(), callback, 0, &error_mes_flag);

        if (add_call != SQLITE_OK) {
            std::cout << "\033[1;31m";
            fprintf(stderr, "SQL error: %s\n", error_mes_flag);
            std::cout << "\033[0m";
            sqlite3_free(error_mes_flag);
        }
        else {
            std::cout << "\033[1;32m";
            fprintf(stdout, "Records created successfully\n");
            std::cout << "\033[0m";
        }
    }
};

class RegularTask : public Task //______________________________________________________________________ ХХХсделать дефолтный конструкторХХХ ______________________________________________________________________
{
private:
    int m_frequency;
public:
    RegularTask(std::string name, int freq) : Task(name), m_frequency(freq)
    {
        m_type = TaskType::REGULAR;
    };
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
            std::cout << "\033[1;31m";
            fprintf(stderr, "SQL error: %s\n", error_mes_flag);
            std::cout << "\033[0m";
            sqlite3_free(error_mes_flag);
        }
        else {
            std::cout << "\033[1;32m";
            fprintf(stdout, "Records created successfully\n");
            std::cout << "\033[0m";
        }
    }
};


// SOME DB FUNK-S
/////////////////////////////////////////////////////////////////////////

void db_open(sqlite3* db, int rc) /* Open database */
{


    if (rc) {
        std::cout << "\033[1;31m";
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        std::cout << "\033[0m";
        exit;
    }
    else {
        std::cout << "\033[1;32m";
        fprintf(stderr, "Opened database successfully\n");
        std::cout << "\033[0m";
    }
}

void tab_set_up(sqlite3* database, char* zErrMsg, int rc)
{
    std::string call_reg = "CREATE TABLE regular_tasks("  \
        "task TEXT NOT NULL UNIQUE," \
        "frequency INTEGER NOT NULL);";

    std::string call_pers = "CREATE TABLE personal_tasks("  \
        "task TEXT NOT NULL UNIQUE);";

    const char* c_call_reg = const_cast<char*>(call_reg.c_str());
    const char* c_call_pers = const_cast<char*>(call_pers.c_str());
    rc = sqlite3_exec(database, c_call_reg, callback, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        std::cout << "\033[1;31m";
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        std::cout << "\033[0m";
        sqlite3_free(zErrMsg);
    }

    else
    {
        std::cout << "\033[1;32m";
        fprintf(stdout, "Operation done successfully\n");
        std::cout << "\033[0m";
    }

    rc = sqlite3_exec(database, c_call_pers, callback_2, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        std::cout << "\033[1;31m";
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        std::cout << "\033[0m";
        sqlite3_free(zErrMsg);
    }

    else
    {
        std::cout << "\033[1;32m";
        fprintf(stdout, "Operation done successfully\n");
        std::cout << "\033[0m";
    }
}

/////////////////////////////////////////////////////////////////////////

std::vector<Task> allmight;

void Dealita(std::string task_name, std::vector<Task>& vec)
{

    for (int i = 0; i < vec.size(); i++)
    {

        if (vec[i].GetName() == task_name) 
        { 
            vec.erase(vec.begin() + i);
            tasks_reg_all.erase(tasks_reg_all.begin() + i);
            tasks_reg_freq.erase(tasks_reg_freq.begin() + i);
            break;
        }
        
    }

    for (int i = 0; i < tasks_reg_null.size(); i++)
    {
        if (tasks_reg_null[i] == task_name)
        {
            tasks_reg_null.erase(tasks_reg_null.begin() + i);
            break;
        }
    }

}

void DealitaJr(std::string task_name, std::vector<Task>& vec)
{
    for (int i = 0; i < vec.size(); i++)
    {

        if (vec[i].GetName() == task_name)
        {
            vec.erase(vec.begin() + i);
            break;
        }


    }

    for (int i = 0; i < tasks_pers.size(); i++)
    {
        if (tasks_pers[i] == task_name)
        {
            tasks_pers.erase(tasks_pers.begin() + i);
            break;
        }
    }

}

void FiltAndFill_reg(int arg, sqlite3* database, char* zErrMsg, int rc)
{
    if (arg < 0)
    {
        std::string filt_call = "SELECT* FROM regular_tasks;";
        const char* filt_tasks = const_cast<char*>(filt_call.c_str());

        rc = sqlite3_exec(database, filt_tasks, callback_5, NULL, &zErrMsg);

        if (rc != SQLITE_OK)
        {
            std::cout << "\033[1;31m";
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            std::cout << "\033[0m";
            sqlite3_free(zErrMsg);
        }

        else
        {
            std::cout << "\033[1;32m";
            fprintf(stdout, "Operation done successfully\n");
            std::cout << "\033[0m";
        }
    }
    else
    {
        std::string filt_call = "SELECT* FROM regular_tasks WHERE frequency=" + std::to_string(arg) + ";";
        const char* filt_tasks = const_cast<char*>(filt_call.c_str());

        rc = sqlite3_exec(database, filt_tasks, callback_3, NULL, &zErrMsg);

        if (rc != SQLITE_OK)
        {
            std::cout << "\033[1;31m";
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            std::cout << "\033[0m";
            sqlite3_free(zErrMsg);
        }

        else
        {
            std::cout << "\033[1;32m";
            fprintf(stdout, "Operation done successfully\n");
            std::cout << "\033[0m";
        }
    }
    
}

void FiltAndFill_pers(sqlite3* database, char* zErrMsg, int rc)
{
    std::string filt_call = "SELECT * FROM personal_tasks";
    const char* filt_tasks = const_cast<char*>(filt_call.c_str());

    rc = sqlite3_exec(database, filt_tasks, callback_4, NULL, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        std::cout << "\033[1;31m";
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        std::cout << "\033[0m";
        sqlite3_free(zErrMsg);
    }

    else
    {
        std::cout << "\033[1;32m";
        fprintf(stdout, "Operation done successfully\n");
        std::cout << "\033[0m";
    }
}


void DeleteRegTask(std::string task_name, sqlite3* database, char* zErrMsg, int rc)
{
    const char* data = "Callback function called";
    std::string task_n = '"' + task_name + '"';

    std::string del_call = "DELETE from regular_tasks WHERE task=" + task_n + ";" \
        "SELECT * from regular_tasks";

    const char* del_task = const_cast<char*>(del_call.c_str());

    rc = sqlite3_exec(database, del_task, callback, (void*)data, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        std::cout << "\033[1;31m";
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        std::cout << "\033[0m";
        sqlite3_free(zErrMsg);
    }

    else
    {

        std::cout << "\033[1;32m";
        fprintf(stdout, "Operation done successfully\n");
        std::cout << "\033[0m";

    }

    Dealita(task_name, allmight);

}

void DeletePersTask(std::string task_name, sqlite3* database, char* zErrMsg, int rc)
{
    const char* data = "Callback function called";
    std::string task_n = '"' + task_name + '"';

    std::string del_call = "DELETE from personal_tasks WHERE task=" + task_n + ";" \
        "SELECT * from regular_tasks";

    const char* del_task = const_cast<char*>(del_call.c_str());

    rc = sqlite3_exec(database, del_task, callback, (void*)data, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        std::cout << "\033[1;31m";
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        std::cout << "\033[0m";
        sqlite3_free(zErrMsg);
    }

    else
    {
        std::cout << "\033[1;32m";
        fprintf(stdout, "Operation done successfully\n");
        std::cout << "\033[0m";

    }

    DealitaJr(task_name, allmight);

}

void Factory(TaskType tt, std::vector<Task> &kuda, std::vector<std::string> &otkuda, std::vector<int> &otkudadop)
{
    PersonalTask temp_pers("sus");
    RegularTask temp_reg("amogass", 1);

    switch (tt)
    {
    case TaskType::PERSONAL:
    {
        int num = otkudadop.size();
        for (int i = num; i < otkuda.size(); i++)
        {
            temp_pers.SetUp(otkuda[i]);
            kuda.push_back(temp_pers);
        }
        break;
    }

    case TaskType::REGULAR:
    {
        int nulm = otkudadop.size();
        for (int i = 0; i < nulm; i++)
        {
            temp_reg.SetUpReg(otkuda[i], otkudadop[i]);
            kuda.push_back(temp_reg);

        }
        break;
    }

    } 
}


int main(int argc, char* argv[])
{

    sqlite3* database;
    char* ErrorMesg = 0;
    int recall_code;
    recall_code = sqlite3_open("real_my_data.db", &database);

    db_open(database, recall_code);
    tab_set_up(database, ErrorMesg, recall_code);

    //Some objects
    RegularTask task1("Watch_One_Piece", 0);
    RegularTask task2("Watch_JoJo", 1);
    RegularTask task3("Watch_J", 3);
    RegularTask task4("Watch One", 3);
    RegularTask task5("Cat", 3);

    PersonalTask ptas1("Fuck");
    PersonalTask ptas2("U");
    PersonalTask ptas3("SESSIA");



    task1.AddToBase(recall_code, database, ErrorMesg);
    task2.AddToBase(recall_code, database, ErrorMesg);
    task3.AddToBase(recall_code, database, ErrorMesg);
    task4.AddToBase(recall_code, database, ErrorMesg);
    task5.AddToBase(recall_code, database, ErrorMesg);


    ptas1.AddToBase(recall_code, database, ErrorMesg);
    ptas2.AddToBase(recall_code, database, ErrorMesg);
    ptas3.AddToBase(recall_code, database, ErrorMesg);

    FiltAndFill_reg(-2, database, ErrorMesg, recall_code); // ВЫЗОВИ ХОТЬ ОДИН РАЗ ОТ arg<0, А ТО ПИЗДЕЦ

    FiltAndFill_reg(3, database, ErrorMesg, recall_code);

    FiltAndFill_pers(database, ErrorMesg, recall_code);


    std::vector<std::string> tasks_all;

    for (int i = 0; i < tasks_reg_null.size(); i++)
    {
        tasks_all.push_back(tasks_reg_null[i]);
    }
    for (int i = 0; i < tasks_pers.size(); i++)
    {
        tasks_all.push_back(tasks_pers[i]);
    }

    for (int i = 0; i < tasks_all.size(); i++)
    {
        std::cout << "YO: " << tasks_all[i] << std::endl;
    }

    Factory(TaskType::REGULAR, allmight, tasks_all, tasks_reg_freq);

    Factory(TaskType::PERSONAL, allmight, tasks_all, tasks_reg_freq);



    DeleteRegTask(allmight[0].GetName(), database, ErrorMesg, recall_code);


    std::cerr << allmight[4].GetName();

    DeletePersTask(allmight[4].GetName(), database, ErrorMesg, recall_code);

    std::cerr << allmight.size() << std::endl;

    std::cerr << allmight[3].GetName();

    sqlite3_close(database);
    return 0;
}
