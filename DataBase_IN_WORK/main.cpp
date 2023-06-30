#include <stdio.h>
#include <stdlib.h>
#include <sqlite3/sqlite3.h> 
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <fstream>


std::string BART = R"(
 |=========================================| 
||..I WILL NOT PROCRASTINATE ANYMORE.......||
||..I WILL NOT PROCRASTINATE ANYMORE.......||
||..I WILL NOT PROCRASTINATE ANYMORE.......||
||..I .-------.PROC,.......................||
||./..><....\ ../..........................||
||.|........| /\...........................||
||.\______/ /\/............................||
||..(____) / /.............................||
||._/ _ . _. ______________________________||
|'.==\___\_).|============================.'|
    .|____|                                  
    | .|| .|                                 
     |_||_|                                  
    (__)(__)                                 

)";

std::string kit = R"(
        `"-.                 .......................................
         )  -.             .   Program for procrastinating cats  .
        ,  : . \           .......................................
        :    '  \
        ; * _.   --.
        -.-'          -.
           |              .
           :.       .        \
           | \  .   :   .-'   .
           :  )-.;  ;  /      :
           :  ;  | :  :       ;-.
           ; /   : |`-:      - )
        ,-' /  ,-' ; .-- .' --'
        --'   ---' ---' )";


std::string text = R"(
  _____                              _   _             _             
 |  __ \                            | | (_)           | |            
 | |__) | __ ___   ___ _ __ __ _ ___| |_ _ _ __   __ _| |_ ___  _ __ 
 |  ___/ '__/ _ \ / __| '__/ _` / __| __| | '_ \ / _` | __/ _ \| '__|
 | |   | | | (_) | (__| | | (_| \__ \ |_| | | | | (_| | || (_) | |   
 |_|   |_|  \___/ \___|_|  \__,_|___/\__|_|_| |_|\__,_|\__\___/|_|                                                                      
)";


//векторы, которые нужны
std::vector<int> tasks_reg_freq_all; //частота, связан с tasks_reg_all
std::vector<std::string> tasks_reg_today; //todays buisness
std::vector<std::string> tasks_reg_all; //все regular таски из bd(без фильтра) [ТРЕБУЕТ ОБЯЗАТЕЛЬНОГО ВЫЗОВА В МЕЙНЕ ОТ -arg в FiltAndFill, иначе пустой]
std::vector<std::string> tasks_pers;//все персональные таски из bd
std::vector<int> tasks_reg_freq_today;

//5 коллбеков
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

    time_t now = time(0);
    tm* ltm = localtime(&now);

protected:

    TaskType m_type = TaskType::NONE;
    std::string m_name;
    int m_frequency;

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

class PersonalTask : public Task
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

    int AddToBase(int add_call, sqlite3* database, char* error_mes_flag)
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
            return 0;
        }
        else {
            std::cout << "\033[1;32m";
            fprintf(stdout, "Records created successfully\n");
            std::cout << "\033[0m";
            return 1;
        }
    }
};

class RegularTask : public Task
{
private:

public:

    RegularTask(std::string name, int freq) : Task(name)
    {
        m_type = TaskType::REGULAR;
        m_frequency = freq;
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

    int AddToBase(int add_call, sqlite3* database, char* error_mes_flag)
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
            return 0;
        }
        else {
            std::cout << "\033[1;32m";
            fprintf(stdout, "Records created successfully\n");
            std::cout << "\033[0m";
            return 1;
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
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
        std::cout << "\033[0m";
        sqlite3_free(zErrMsg);
    }
    else
    {
        std::cout << "\033[1;32m";
        fprintf(stdout, "Table set up successfully (regular tasks table)\n");
        std::cout << "\033[0m";
    }

    rc = sqlite3_exec(database, c_call_pers, callback_2, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        std::cout << "\033[1;31m";
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
        std::cout << "\033[0m";
        sqlite3_free(zErrMsg);
    }
    else
    {
        std::cout << "\033[1;32m";
        fprintf(stdout, "Table set up successfully (personal tasks table)\n");
        std::cout << "\033[0m";
    }
}



/////////////////////////////////////////////////////////////////////////

std::vector<Task> task_vec_for_all;

void Dealita_reg_from_task_vec(std::string task_name, std::vector<Task>& vec) //системная ф 
{
    bool flag = false;

    for (int i = 0; i < vec.size(); i++)
    {

        if (vec[i].GetName() == task_name)
        {
            vec.erase(vec.begin() + i);
            tasks_reg_all.erase(tasks_reg_all.begin() + i);
            tasks_reg_freq_all.erase(tasks_reg_freq_all.begin() + i);
            flag = true;
            break;
        }

    }

    if (flag == false) 
    {
        std::cout << "\033[0;35m";
        std::cout << "Такого дела не существует!" << std::endl;
        std::cout << "\033[0m";
    }
}


void Dealita_pers_from_task_vec(std::string task_name, std::vector<Task>& vec) //системная ф
{
    bool flag = false;
    for (int i = 0; i < vec.size(); i++)
    {

        if (vec[i].GetName() == task_name)
        {
            vec.erase(vec.begin() + i);
            flag = true;
            break;
        }
    }
    if (flag == true)
    {
        for (int i = 0; i < tasks_pers.size(); i++)
        {
            if (tasks_pers[i] == task_name)
            {
                tasks_pers.erase(tasks_pers.begin() + i);
                break;
            }
        }
    }
    else if (flag == false)
    {
        std::cout << "\033[0;35m";
        std::cout << "Такого дела не существует!" << std::endl;
        std::cout << "\033[0m";
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
            //fprintf(stdout, "Operation done successfully\n");
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
            //fprintf(stdout, "Operation done successfully\n");
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
        //fprintf(stdout, "Operation done successfully\n");
        std::cout << "\033[0m";
    }
}


void DeleteRegTask(std::string task_name, sqlite3* database, char* zErrMsg, int rc) //системная ф
{
    const char* data = "Callback function called";
    std::string task_n = '"' + task_name + '"';

    std::string del_call = "DELETE from regular_tasks WHERE task=" + task_n + ";";

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
        fprintf(stdout, "Deleted successfully\n");
        std::cout << "\033[0m";

    }
    Dealita_reg_from_task_vec(task_name, task_vec_for_all);
}

void DeletePersTask(std::string task_name, sqlite3* database, char* zErrMsg, int rc) //системная ф

{
    const char* data = "Callback function called";
    std::string task_n = '"' + task_name + '"';
    std::string del_call = "DELETE from personal_tasks WHERE task=" + task_n + ";";
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
        fprintf(stdout, "Deleted successfully\n");
        std::cout << "\033[0m";
    }
    Dealita_pers_from_task_vec(task_name, task_vec_for_all);
}


void Factory(TaskType tt, std::vector<Task>& kuda, std::vector<std::string>& otkuda, std::vector<int>& otkudadop) //фабрика объектов
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


void CountUp(int cycles, sqlite3* database, char* zErrMsg, int rc) // прибавляет единичку к счётчикам
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
        fprintf(stdout, "Frequency calibrated successfully\n");
        std::cout << "\033[0m";

    }
}

void Checker(sqlite3* db, char* zErrMsg, int rc) //зануляет счётчики, превысившие свою частоту
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
        std::cout << "\033[1;31m";
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
        std::cout << "\033[0m";
    }

    sqlite3_bind_int(stmt, 1, '0');

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cout << "\033[1;31m";
        std::cerr << "Failed to execute update statement: " << sqlite3_errmsg(db) << std::endl;
        std::cout << "\033[0m";
    }

    sqlite3_reset(stmt);
} 


// TIME SECTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

int TimeUpdate()  //сверяет дату последней сессии  и перезаписывает её (возвращает сколько дней прошло с последней сессии)
{
    // txt и немного 
    std::ifstream in("last_session.txt", std::ios::in);
    std::string last_date;
    std::string today_date;
    in >> last_date;
    std::string tempy = "";

    tempy += last_date[0];
    tempy += last_date[1];
    int tod_day = atoi(tempy.c_str());

    tempy = "";
    tempy += last_date[3];
    tempy += last_date[4];
    int tod_mon = atoi(tempy.c_str());

    tempy = "";
    tempy += last_date[6];
    tempy += last_date[7];
    tempy += last_date[8];
    tempy += last_date[9];
    int tod_ye = atoi(tempy.c_str());


    tm last_ses = { 0,0,0, tod_day,tod_mon-1,tod_ye - 1900 };

    time_t now = time(0);
    tm* ltm = localtime(&now);

    tm now_ses = { 0, 0, 0, ltm->tm_mday, ltm->tm_mon,ltm->tm_year };
    std::time_t start_time = std::mktime(&last_ses);
    std::time_t end_time = std::mktime(&now_ses);

    double seconds = std::difftime(end_time, start_time);
    int days = seconds / (60 * 60 * 24);

    tempy = "";
    if ((ltm->tm_mday) / 10 != 0){ tempy += std::to_string(ltm->tm_mday);}
    if ((ltm->tm_mday) / 10 == 0) { tempy = tempy + "0" + std::to_string(ltm->tm_mday);}
    tempy += "/";
    if ((ltm->tm_mon + 1) / 10 != 0) { tempy += std::to_string(ltm->tm_mon + 1); }
    if ((ltm->tm_mon + 1) / 10 == 0) { tempy = tempy + "0" + std::to_string(ltm->tm_mon + 1);}
    tempy += "/"; 
    tempy += std::to_string(ltm->tm_year+1900);

    std::ofstream out_t("last_session.txt", std::ios::out);
    out_t << tempy;
    return days;
}

void CleanUp_Pers(int day_dif, sqlite3* database, char* zErrMsg, int rc)   // очистка от персоналов, оставшихся со вчера)
{
    if (day_dif != 0)
    {
        int y = tasks_pers.size();
        for (int i = 0; i < y; i++)
        {
            DeletePersTask(tasks_pers[0], database, zErrMsg, rc);
            std::cout << "All one-time cases from past sessions have been successfully deleted" << std::endl;
        }
    }
}

void Find_and_Edit(std::string old_name, std::string new_name, int new_freq, sqlite3* database, char* zErrMsg, int rc)
{
    bool flag = false;
    for (int i = 0; i < task_vec_for_all.size(); i++)
    {
        if (task_vec_for_all[i].GetName() == old_name)
        {
            if (task_vec_for_all[i].GetType() == TaskType::PERSONAL)////////////
            {
                task_vec_for_all[i].SetUp(new_name); 

                /// base update 
                std::string d_old_name = "";
                for (int v = 0; v < old_name.length(); v++) { d_old_name += old_name[v]; }

                std::string d_new_name = "";
                for (int v = 0; v < new_name.length(); v++) { d_new_name += new_name[v]; }

                new_name = '"' + new_name + '"';
                old_name = '"' + old_name + '"';

                std::string sql = "UPDATE personal_tasks SET task =" + new_name + " WHERE task = " + old_name + ";";
                rc = sqlite3_exec(database, sql.c_str(), callback, 0, &zErrMsg);

                if (rc != SQLITE_OK)
                {
                    std::cout << "\033[1;31m";
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    std::cout << "\033[0m";
                    sqlite3_free(zErrMsg);
                }
                else
                {
                    for (int y = 0; y < tasks_pers.size(); y++) { if (tasks_pers[y] == d_old_name) { tasks_pers[y] = d_new_name; } }
                    std::cout << "\033[1;32m";
                    fprintf(stdout, "Base updated successfully (personal)\n");
                    std::cout << "\033[0m";
                }
            }
            if (task_vec_for_all[i].GetType() == TaskType::REGULAR)////////////
            {
                static_cast<RegularTask&>(task_vec_for_all[i]).SetUpReg(new_name, new_freq);
                for (int y = 0; y < tasks_reg_all.size(); y++)
                {
                    if (tasks_reg_all[y] == old_name)
                    {
                        tasks_reg_all[y] = new_name;
                        tasks_reg_freq_all[y] = new_freq;

                        /// base update

                        new_name = '"' + new_name + '"';
                        old_name = '"' + old_name + '"';

                        std::string sql = "UPDATE regular_tasks SET frequency=" + std::to_string(new_freq) + " WHERE task="  + old_name + ';';
                        rc = sqlite3_exec(database, sql.c_str(), callback, 0, &zErrMsg);
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
                            fprintf(stdout, "Base updated successfully (regular, frequency)\n");
                            std::cout << "\033[0m";
                        }

                        sql = "UPDATE regular_tasks SET task=" + new_name  + " WHERE task=" + old_name + ';';
                        rc = sqlite3_exec(database, sql.c_str(), callback, 0, &zErrMsg);
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
                            fprintf(stdout, "Base updated successfully (regular, name)\n");
                            std::cout << "\033[0m";
                        }
                    }
                }
            }
            flag = true;
        }
        
    }

    if (flag == false)
    {
        std::cout << "\033[0;35m";
        std::cout << "Такого дела не существует!" << std::endl;
        std::cout << "\033[0m";
    }
}



int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "ru");
    std::vector<RegularTask> session_reg;
    std::vector<PersonalTask> session_pers;
    int day_dif;
    char choose;

    //декоративные штуки
    std::cout << text << std::endl;
    std::cout << kit << std::endl;
    std::cout << std::endl;


    sqlite3* database;
    char* ErrorMesg = 0;
    int recall_code;
    recall_code = sqlite3_open("real_my_data.db", &database);
    // setup func:
    db_open(database, recall_code);
    tab_set_up(database, ErrorMesg, recall_code);
    
    bool key = true;

    // Формирование векторов из старых записей ДБ
    FiltAndFill_reg(-2, database, ErrorMesg, recall_code);
    FiltAndFill_pers(database, ErrorMesg, recall_code);
    std::vector<std::string> tasks_all;

    for (int i = 0; i < tasks_reg_all.size(); i++)
    {
        tasks_all.push_back(tasks_reg_all[i]);
    }
    for (int i = 0; i < tasks_pers.size(); i++)
    {
        tasks_all.push_back(tasks_pers[i]);
    }

    Factory(TaskType::REGULAR, task_vec_for_all, tasks_all, tasks_reg_freq_all);

    Factory(TaskType::PERSONAL, task_vec_for_all, tasks_all, tasks_reg_freq_all);

    day_dif = TimeUpdate();
    CleanUp_Pers(day_dif, database, ErrorMesg, recall_code);

    for (int i = 0; i < day_dif; i++)
    {
        CountUp(1, database, ErrorMesg, recall_code);
        Checker(database, ErrorMesg, recall_code);
    }

    while (key == true)
    {
         day_dif = TimeUpdate();
         Checker(database, ErrorMesg, recall_code);
         if (day_dif != 0) 
         {
             CleanUp_Pers(1, database, ErrorMesg, recall_code); 
             CountUp(1, database, ErrorMesg, recall_code);
         }

         std::cout << "Варианты действия: \n[A] Взаимодейcтвия с единоразовыми делами.  \n[B] Взаимодействия с регулярными делами. \n[Q] Выйти из программы." << std::endl;
         std::cin >> choose;

         if (choose == 'A')
         {
             std::cout << "Варианты действия: \n[A] Добавить объект.  \n[B] Удалить объект.  \n[C] Изменить объект.\n[Q] Назад." << std::endl;
             std::cin >> choose;
             switch (choose)
             {
             case('A'):
             {
                 std::string comand;
                 std::cout << "Введите название дела:" << std::endl;
                 std::cin.ignore();
                 std::getline(std::cin, comand);
                 PersonalTask temp(comand);
                 int flag1 = temp.AddToBase(recall_code, database, ErrorMesg);
                 if (flag1 == 1)
                 {
                     task_vec_for_all.push_back(temp);
                     tasks_pers.push_back(comand);
                 }
                 
                 break;
             }
             case('B'):
             {
                 std::string comandr;
                 std::cout << "Введите название дела, которое нужно удалить:" << std::endl;
                 std::cin.ignore();
                 std::getline(std::cin, comandr);
                 DeletePersTask(comandr, database, ErrorMesg, recall_code);
                 break;
             }
             case('C'):
             {
                 std::string comd1;
                 std::string comd2;
                 std::cout << "Введите название дела, которое нужно изменить:" << std::endl;
                 std::cin.ignore();
                 std::getline(std::cin, comd1);
                 std::cout << "Введите новое название дела:" << std::endl;
                 //std::cin.ignore();
                 std::getline(std::cin, comd2);
                 Find_and_Edit(comd1, comd2, -1, database, ErrorMesg, recall_code);
                 break;

             }
             case('Q'):{break;}
             choose = 'W';
             }

         }
         else if (choose == 'B')
         {
             std::cout << "Варианты действия: \n[A] Добавить объект.  \n[B] Удалить объект.  \n[C] Изменить объект.\n[Q] Назад." << std::endl;
             std::cin >> choose;
             switch (choose)
             {
             case('A'):
             {
                 std::string comand1;
                 int comand2;
                 std::cout << "Введите название дела:" << std::endl;
                 std::cin.ignore();
                 std::getline(std::cin, comand1);
                 std::cout << "Введите частоту выполнения дела (раз в сколько дней):" << std::endl;
                 std::cin >> comand2;
                 RegularTask temp(comand1,comand2);
                 int flag2 = temp.AddToBase(recall_code, database, ErrorMesg);
                 if (flag2 == 1)
                 {
                     task_vec_for_all.push_back(temp);
                     tasks_reg_all.push_back(comand1);
                     tasks_reg_freq_all.push_back(comand2);
                 }
                 break;
             }
             case('B'):
             {
                 std::string comandr;
                 std::cout << "Введите название дела, которое нужно удалить:" << std::endl;
                 std::cin.ignore();
                 std::getline(std::cin, comandr);
                 DeleteRegTask(comandr, database, ErrorMesg, recall_code);
                 break;
             }
             case('C'):
             {
                 std::string comd1;
                 std::string comd2;
                 int comd3;
                 std::cout << "Введите название дела, которое нужно изменить:" << std::endl;
                 std::cin.ignore();
                 std::getline(std::cin, comd1);
                 std::cout << "Введите новое название дела:" << std::endl;
                 std::getline(std::cin, comd2);
                 std::cout << "Введите новую частоту выполнения дела (раз в сколько дней):" << std::endl;
                 std::cin >> comd3;
                 Find_and_Edit(comd1, comd2, comd3, database, ErrorMesg, recall_code);
                 break;
             }
             case('Q'): {break; }
             choose = 'W';
             }
             choose = 'W';
         }
         else if (choose == 'Q')
         {
             key == false;
             break;
         }
    }

    // Формирование векторов из старых записей ДБ
    FiltAndFill_reg(-2, database, ErrorMesg, recall_code); // ВЫЗОВИ ХОТЬ ОДИН РАЗ ОТ arg<0, А ТО /*очень грустно*/
    FiltAndFill_reg(0, database, ErrorMesg, recall_code);

    std::ofstream out("today_list.txt", std::ios::out);

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

    //std::cout << "\033[47m";
    //std::cout << "\033[30m";
    std::cout << BART << std::endl;
    //std::cout << "\033[0m";
    std::cout << std::endl;

    
    sqlite3_close(database);
    system("pause");

    return 0;
}
