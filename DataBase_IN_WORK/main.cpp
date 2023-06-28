#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3/sqlite3.h> 
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <fstream>

//векторы, которые нужны
std::vector<int> tasks_reg_freq_all; //частота, связан с tasks_reg_all
std::vector<std::string> tasks_reg_today; //todays buisness
std::vector<std::string> tasks_reg_all; //все regular таски из bd(без фильтра) [ТРЕБУЕТ ОБЯЗАТЕЛЬНОГО ВЫЗОВА В МЕЙНЕ ОТ -arg в FiltAndFill, иначе пустой]
std::vector<std::string> tasks_pers;//все персональные таски из bd
std::vector<int> tasks_reg_freq_today;

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
    for (int i = 0; i < argc; i++)
    {
        if (i % 3 == 0)
        {
            tasks_reg_today.push_back(argv[i]);
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
        if (i % 3 == 0)
        {
            tasks_reg_all.push_back(argv[i]);
        }
        else if (i % 3 == 1)
        {
            tasks_reg_freq_all.push_back(static_cast<int>(*argv[i]));
        }

    }
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
    TaskType m_type = TaskType::NONE;

    std::string m_name;

public:
    Task(std::string name) : m_name(name) {};

    Task() = default;

    Task(const Task& other) : Task(other.m_name)
    {
        this->m_type = other.m_type;
    }

    Task& operator = (const Task& other)
    {
        Task tmp = other;
        std::swap(this->m_name, tmp.m_name);
        std::swap(this->m_type, tmp.m_type);
        return *this;
    }



    virtual ~Task() = default;

    virtual void SetUp(std::string name)
    {
        m_name = name;
    };

    virtual void ShowInfo()
    {
        std::cout << "Name of task: " << m_name << std::endl;
    }

    virtual std::string GetName()
    {
        return m_name;
    }

    virtual TaskType GetType() { return m_type; }
};

class PersonalTask : public Task //______________________________________________________________________ ХХХсделать дефолтный конструкторХХХ ______________________________________________________________________
{
private:
public:
    PersonalTask(std::string name) : Task(name) { m_type = TaskType::PERSONAL; }
    PersonalTask() = default;

    PersonalTask(const PersonalTask& other) : PersonalTask(other.m_name)
    {
        this->m_type = other.m_type;
    }

    PersonalTask& operator = (const PersonalTask& other)
    {
        PersonalTask tmp = other;
        std::swap(this->m_name, tmp.m_name);
        std::swap(this->m_type, tmp.m_type);
        return *this;
    }


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
    int m_frequency = 0;
public:
    RegularTask(std::string name, int freq) : Task(name), m_frequency(freq)
    {
        m_type = TaskType::REGULAR;
    };

    RegularTask() = default;

    ~RegularTask() = default;

    RegularTask(const RegularTask& other) : RegularTask(other.m_name, other.m_frequency)
    {
        this->m_type = other.m_type;
    }

    RegularTask& operator = (const RegularTask& other)
    {
        RegularTask tmp = other;
        std::swap(this->m_name, tmp.m_name);
        std::swap(this->m_type, tmp.m_type);
        return *this;
    }

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

void db_open(sqlite3* db, int rc) /* Open database */  //системная ф
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

void tab_set_up(sqlite3* database, char* zErrMsg, int rc) //системная ф
{
    std::string call_reg = "CREATE TABLE regular_tasks("  \
        "task TEXT NOT NULL UNIQUE," \
        "frequency INTEGER NOT NULL," \
        "counter INTEGER DEFAULT 0);";

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

std::vector<Task> task_vec_for_all;

void Dealita_reg_from_task_vec(std::string task_name, std::vector<Task>& vec) //системная ф 
{

    for (int i = 0; i < vec.size(); i++)
    {

        if (vec[i].GetName() == task_name)
        {
            vec.erase(vec.begin() + i);
            tasks_reg_all.erase(tasks_reg_all.begin() + i);
            tasks_reg_freq_all.erase(tasks_reg_freq_all.begin() + i);
            break;
        }

    }

    for (int i = 0; i < tasks_reg_today.size(); i++)
    {
        if (tasks_reg_today[i] == task_name)
        {
            tasks_reg_today.erase(tasks_reg_today.begin() + i);
            break;
        }
    }
}

void Dealita_from_other_vec(std::string task_name, TaskType typ)
{
    switch (typ)
    {
    case(TaskType::REGULAR):
    {
        for (int i = 0; i < tasks_reg_all.size(); i++)
        {
            if (tasks_reg_all[i] == task_name)
            {
                tasks_reg_all.erase(tasks_reg_all.begin() + i);
                tasks_reg_freq_all.erase(tasks_reg_freq_all.begin() + i);
            }
        }
        for (int i = 0; i < tasks_reg_today.size(); i++)
        {
            if (tasks_reg_today[i] == task_name)
            {
                tasks_reg_today.erase(tasks_reg_today.begin() + i);
                tasks_reg_freq_today.erase(tasks_reg_freq_today.begin() + i);
            }
        }
    }
    case(TaskType::PERSONAL):
    {
        for (int i = 0; i < tasks_pers.size(); i++)
        {
            if (tasks_pers[i] == task_name)
            {
                tasks_pers.erase(tasks_pers.begin() + i);
            }
        }
    }

    }
}

void Dealita_pers_from_task_vec(std::string task_name, std::vector<Task>& vec) //системная ф
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


void FiltAndFill_reg(int arg, sqlite3* database, char* zErrMsg, int rc) //системная ф
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
        std::string filt_call = "SELECT* FROM regular_tasks WHERE counter=" + std::to_string(arg) + ";";
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

void FiltAndFill_pers(sqlite3* database, char* zErrMsg, int rc)  //системная ф
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


void DeleteRegTask(std::string task_name, sqlite3* database, char* zErrMsg, int rc) //системная ф
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

    Dealita_reg_from_task_vec(task_name, task_vec_for_all);
    Dealita_from_other_vec(task_name, TaskType::REGULAR);
}

void DeletePersTask(std::string task_name, sqlite3* database, char* zErrMsg, int rc) //системная ф

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

    Dealita_pers_from_task_vec(task_name, task_vec_for_all);
    Dealita_from_other_vec(task_name, TaskType::PERSONAL);

}


void Factory(TaskType tt, std::vector<Task>& kuda, std::vector<std::string>& otkuda, std::vector<int>& otkudadop) //системная ф
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


void CountUp(int cycles, sqlite3* database, char* zErrMsg, int rc)
{
    std::string str = "UPDATE regular_tasks SET counter = counter + " + std::to_string(cycles);
    const char* req = const_cast<char*>(str.c_str());
    rc = sqlite3_exec(database, req, callback, 0, &zErrMsg);

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

void Checker(sqlite3* db, char* zErrMsg, int rc)
{
    const char* query = "UPDATE regular_tasks SET counter = 0 WHERE counter >= frequency";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
    }

    int result = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int freq = sqlite3_column_int(stmt, 0);
        int count = sqlite3_column_int(stmt, 1);

        if (count >= freq)
        {
            const char* updateQuery = "UPDATE regular_tasks SET counter = ? WHERE frequency = counter";
            rc = sqlite3_prepare_v2(db, updateQuery, -1, &stmt, nullptr);
        }
    }

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_bind_int(stmt, 1, '0');

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "Failed to execute update statement: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_reset(stmt);
}


void Edit(std::vector<Task>& origin, std::string what_to_edit, TaskType what_type) //from Task-Vector
{

}

void Delete(std::vector<Task>& origin) //from Task-Vector
{

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
    RegularTask task1("Watch_One_Piece", 1);
    RegularTask task2("Watch_JoJo", 1);
    RegularTask task3("Watch_J", 2);
    RegularTask task4("Watch One", 3);
    RegularTask task5("Cat", 3);

    PersonalTask ptas1("Fuck");
    PersonalTask ptas2("U");
    PersonalTask ptas3("SESSIA");

    RegularTask task6 = task5;
    task6.SetUp("catzzz");
    task6.AddToBase(recall_code, database, ErrorMesg);

    task1.AddToBase(recall_code, database, ErrorMesg);
    task2.AddToBase(recall_code, database, ErrorMesg);
    task3.AddToBase(recall_code, database, ErrorMesg);
    task4.AddToBase(recall_code, database, ErrorMesg);
    task5.AddToBase(recall_code, database, ErrorMesg);


    ptas1.AddToBase(recall_code, database, ErrorMesg);
    ptas2.AddToBase(recall_code, database, ErrorMesg);
    ptas3.AddToBase(recall_code, database, ErrorMesg);



    // Формирование векторов из старых записей ДБ
    FiltAndFill_reg(-2, database, ErrorMesg, recall_code); // ВЫЗОВИ ХОТЬ ОДИН РАЗ ОТ arg<0, А ТО ПИЗДЕЦ

    FiltAndFill_reg(0, database, ErrorMesg, recall_code);

    FiltAndFill_pers(database, ErrorMesg, recall_code);
    //

    CountUp(1, database, ErrorMesg, recall_code);

    std::vector<std::string> tasks_all;

    for (int i = 0; i < tasks_reg_all.size(); i++)
    {
        tasks_all.push_back(tasks_reg_all[i]);
    }
    for (int i = 0; i < tasks_pers.size(); i++)
    {
        tasks_all.push_back(tasks_pers[i]);
    }


    for (int i = 0; i < tasks_all.size(); i++)
    {
        std::cout << "YO: " << tasks_all[i] << std::endl;
    }
    //
    Factory(TaskType::REGULAR, task_vec_for_all, tasks_all, tasks_reg_freq_all);

    Factory(TaskType::PERSONAL, task_vec_for_all, tasks_all, tasks_reg_freq_all);

    //std::cerr << "FIRST DEL:" << task_vec_for_all[1].GetName() << std::endl;
    //DeleteRegTask(task_vec_for_all[1].GetName(), database, ErrorMesg, recall_code);


    //std::cerr << task_vec_for_all[4].GetName();
    //std::cerr << "SECOND DEL:" << task_vec_for_all[2].GetName() << std::endl;
    //DeleteRegTask(task_vec_for_all[2].GetName(), database, ErrorMesg, recall_code);

    //std::cerr << task_vec_for_all.size() << std::endl;

   // std::cerr << task_vec_for_all[3].GetName();





    // txt и немного 
    std::ifstream in("last_session.txt", std::ios::in);
    std::ofstream out("today_list.txt", std::ios::out);

    std::string last_date;
    std::string today_date;

    in >> last_date;

    /*if (last_date != today_date)
    {
        for (int i = 0; i < tasks_pers.size(); i++)
        {
            DeletePersTask()
        }
    }*/

   

    Checker(database, ErrorMesg, recall_code);

    for (int j = 0; j < tasks_reg_today.size(); j++)
    {
        out << tasks_reg_today[j];
        out << std::endl;
    }

    for (int j = 0; j < tasks_pers.size(); j++)
    {
        out << tasks_pers[j];
        out << std::endl;
    }


    sqlite3_close(database);
    return 0;
}
