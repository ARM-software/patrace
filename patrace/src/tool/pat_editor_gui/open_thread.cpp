#include "open_thread.h"
#include <QThread>
#include <QStringList>
#include <fstream>
#include <string>

bool Pat_reading = false;
bool Pat_saving = false;

int pat_extract(const std::string &source_name, const std::string &target_name, int begin_call = 0, int end_call = std::numeric_limits<int>::max());
int merge_to_pat(const std::string &source_name, const std::string &target_name);

OpenJson_Worker::OpenJson_Worker(QObject *parent):
    QObject(parent)
{}

OpenJson_Worker::~OpenJson_Worker()
{}

void OpenJson_Worker::openJson(const QStringList &filenames)
{
    for (int i = 0; i < filenames.size(); i++)
    {
        QString filename = filenames[i];
        std::ifstream fin(filename.toStdString().c_str());
        char c;
        QStringList result;
        std::string s;
        bool started = false;
        int current_bracket_stack = 0;

        while (fin.get(c))
        {
             if (c == '{')
             {
                 started = true;
                 ++current_bracket_stack;
             }
             if (started == true)
             {
                 s.push_back(c);
                 if (c == '}')
                 {
                    --current_bracket_stack;
                    if (current_bracket_stack == 0)
                    {
                        started = false;
                        QString stdtoQstring = QString::fromStdString(s);
                        s.clear();
                        result.append(stdtoQstring);
                    }
                }
            }
        }
        fin.close();
        int length = result.size();
        emit resultReady(result, length, i);
    }
}

void OpenJson_Worker::extractPat(const QString &filename_pat, const QString &dirname_pat)
{
    std::string source_name = filename_pat.toStdString();
    std::string target_name = dirname_pat.toStdString();
    pat_extract(source_name, target_name); //resolve the pat and get json file
    Pat_reading = false;
    emit extractFinish();
}

void OpenJson_Worker::mergePat(const QString &dirname_pat, const QString &filename_pat)
{
    std::string source_name = dirname_pat.toStdString();
    std::string target_name = filename_pat.toStdString();
    merge_to_pat(source_name, target_name);
    Pat_saving =false;
}
