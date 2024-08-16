/* Programik biologic2kintek

sluzy konwersji plikow generowanych przez program BIO-KINE
do plikow *txt czytanych przez KinTek

autor: tomasz.borowski@ikifp.edu.pl

ostatnia modyfikacja: 15.08.2024
*/
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <sys/stat.h>
using namespace std;
//**********************
void info()
{
    cout << "INFO: program nalezy wywolac z dwoma argumentami:\n"
    "1) nazwa pliku inputowego (typu bk3a zapisanego przez BIO-KINE) \n"
    "2) nazwa pliku wynikowego (typu txt, czytanego przez KinTek) \n";
}
size_t pozycjonuj_po_etykiecie(istringstream & s, string etykietka);
bool szukacz(istringstream & s, string etykietka, string & opis);

//***********************
int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        cout << "Bledna liczba argumentow wywolania programu!\n\n";
        info();
        return 5;
    }
    string inp_f_name = (string) argv[1];
    string out_f_name = (string) argv[2];
    if(inp_f_name == out_f_name)
    {
        cout << "Blad: nazwy plikow inputowego i outputowego sa identyczne!\n";
        info();
        return 4;
    }

    cout << "##############################################\n";
    cout << "Program: " << argv[0] << endl;
    cout << "Bede czytal z pliku: " << inp_f_name << endl;
    cout << "a zapisywal plik: " << out_f_name << endl;
    cout << "##############################################\n";

// Czytanie z pliku bk3a do stringu inp_f_tresc:
    ifstream bk3a_file(inp_f_name);
    if(!bk3a_file)
    {
        cout << "Nie mogle otworzyc pliku: " << inp_f_name << endl;
        return 1;
    }
    string linijka;
    string inp_f_tresc;
    while (getline(bk3a_file, linijka))
    {
        inp_f_tresc += (linijka + '\n');
    }
    bk3a_file.close();

    istringstream s_inp_f_tresc(inp_f_tresc);
// odczyt formatu pliku:
    string format;
    if(szukacz(s_inp_f_tresc, "_FORMAT", format))
    {
        int format_length = format.length();
        string format_n;
        for(int i=1; i<format_length-1; i++)
        {
            format_n += format[i];
        }
        format = format_n;
        cout << "Format pliku " << inp_f_name << " to: " << format_n << endl;
    }
    else
    {
        cout << "Blad: Nie udalo sie odczytac formatu danych w pliku\n";
        return 1;
    }

// odczyt sekcji _DATA z pliku inputowego bk3a:
    double W, T, V;
    vector<double> W_vec;
    vector<double> T_vec;
    vector<vector<double>> V_tab;
    vector<double> temp_raw;
    int nr_lini_data = 0;
    double poprzednia_W = -999.9;
    double max_T = -9.9;
    double V_min = 9999.9;
    double V_max = -9999.9;
    if(s_inp_f_tresc.seekg(pozycjonuj_po_etykiecie(s_inp_f_tresc, "_DATA") ) )
    {
        if(format == "WTV")
        {
            while (s_inp_f_tresc >> W >> T >> V)
            {
                nr_lini_data += 1;
              //  cout << W << " " << T << " " << V << endl; //tutaj bedzie zapisywanie danych
                if((W != poprzednia_W) and (poprzednia_W > 0.0))
                {
                    poprzednia_W = W;
                    W_vec.push_back(W);
                    V_tab.push_back(temp_raw);
                    temp_raw.clear();
                }else if ((W != poprzednia_W) and (poprzednia_W < 0.0))
                {
                    poprzednia_W = W;
                    W_vec.push_back(W);
                }
                
                if(T > max_T)
                {
                    T_vec.push_back(T);
                    max_T = T;
                }

                temp_raw.push_back(V);
                if (V > V_max) V_max = V;
                if (V < V_min) V_min = V;

            }
            V_tab.push_back(temp_raw); //final raw of data

            if(s_inp_f_tresc.fail())
            {
                if(s_inp_f_tresc.eof())
                {
                    cout << "Przeczytalem plik do konca, " << nr_lini_data << " punktow danych\n";
                }
                else
                {
                    cout << "Problem z odczytem " << (nr_lini_data + 1) << " linii w bloku _DATA \n";
                }
            }
        }
        else
        {
            cout << "Nieznany mi jeszcze format pliku inputowego: " << format << endl;
            return 1;
        }
    }
    else
    {
        cout << "Blad: Nie udalo sie znalezc bloku _DATA w pliku\n";
        return 1;
    }
    
/*     cout << "wektor W_vec ma: " << W_vec.size() << " elementow\n";
    cout << "wektor T_vec ma: " << T_vec.size() << " elementow\n";
    cout << "tablica V_tab ma: " << V_tab.size() << " wierszy\n";
    cout << "kazdy z tych wierszy ma elementow: \n";
    for(auto wiersz : V_tab)
    {
        cout << wiersz.size() << endl;
    } */

    cout << "zakres dlugosci fali to od: " << W_vec[0] << " do " <<  W_vec[W_vec.size()-1] << " nm" << endl;
    cout << "zakres czasow pomiarow to od: " << T_vec[0] << " do " <<  T_vec[T_vec.size()-1] << " s" << endl;
    cout << "zakres wartosci pomiaru od: " << V_min << " do " << V_max << endl;

// zapis danych do pliku wynikowego
    struct stat buffer;
    int         status;
    status = stat(argv[2], &buffer);
    string decyzja="n";
    if(status == 0)
    {
        cout << "Plik " << out_f_name << " juz istnieje; czy mam go nadpisac? [t/n]\n";
        cin >> decyzja;
    }

    if( (status == -1) or (decyzja == "t"))
    {
        ofstream txt_file(out_f_name);
        if(!txt_file)
        {
            cout << "Nie mogle otworzyc pliku: " << out_f_name << endl;
            return 1;
        }

        // pierwsza linia outputu - "Wave\time + czasy pomiarow"
        ostringstream s_linijka;
        string w_time = "Wave\\time";
        s_linijka << w_time;
        for(auto ele : T_vec)
        {
            s_linijka << "\t" << ele;
        }
        s_linijka << endl;
        txt_file << s_linijka.str();
        s_linijka.str("");
        
        // pozostale linie - z wartosciami wave i wynikow pomiarow (V_tab)
        int l_wierszy = V_tab.size();
        int l_kolumn = T_vec.size();
        for(int i = 0; i < l_wierszy; i++)
        {
            s_linijka.str("");
            s_linijka << W_vec[i];
            for(int j = 0; j < l_kolumn; j++)
            {
                s_linijka << setprecision(7) << "\t" << V_tab[i][j];
            }
            s_linijka << endl;
            txt_file << s_linijka.str();
        }

        txt_file.close();
    }
    else
    {
        return 1;
    }
}
//***********************
size_t pozycjonuj_po_etykiecie(istringstream & s, string etykietka)
{
    string tr = s.str();
    size_t nr = tr.find(etykietka);
    if(nr == string::npos)
    {
        cout << etykietka << " nie znaleziona" << endl;
        return 0;
    }
    return (nr + 1 + etykietka.length());
}

bool szukacz(istringstream & s, string etykietka, string & opis)
{
    s.seekg(pozycjonuj_po_etykiecie(s, etykietka) );
    string local_opis;
    s >> local_opis;
    if(!s)
    {
        cout << "Blad wczytywania tresci po etykietce: " << etykietka << endl;
        return false;
    }
    else
    {
        opis = local_opis;
        return true;
    }
}