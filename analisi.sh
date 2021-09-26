#!/bin/bash
pid=$(pidof prova)
 while [[ -d /proc/$pid ]]; do
     #ps -l
     sleep 1
 done

if [ $# -lt 1 ]; then                       #controllo che ci siano abbasranza argomenti
    echo "non ci sono abbastanza argomenti"
    exit -1
fi
if [ ! -e $1 ]; then                        #controllo che il file esista
    echo "il file è inesistente"
    exit -1
fi

values=() # array dei valori dei clienti
Cvalues=() #array dei valori delle casse
names=(ID PA TS TC CV)
Cnames=(ID NCust NClosure TMS PE TTA)
count=0
customersFlag=0
cashesFlag=0
flag=0

exec 3<$1
while read -u 3 line; do
    name=$(echo $line | cut -f 1 -d ':') #nome
    ##########################################
    if [[ $name == "CLIENTE" ]]; then
        if [[ $customersFlag -eq 0 ]]; then
            echo ""
            echo "DATI DEI CLIENTI"
        fi
        values[count]=$(echo $line | cut -f 2 -d ':') #valore
        (( count++ ))
        customersFlag=1
    elif [[ $name == "CASSA" ]]; then
        flag=1
        if [[ $cashesFlag -eq 0 ]]; then
            echo "DATI DELLE CASSE"
        fi
        Cvalues[count]=$(echo $line | cut -f 2 -d ':') #valore
        (( count++ ))
        cashesFlag=1
    fi
    ##########################################
    case $name in
        "PRODOTTI ACQUISTATI")
            values[count]=$(echo $line | cut -f 2 -d ':')
            (( count++ ))
            ;;
        "TEMPO NEL SUPERMERCATO"|"TEMPO IN CODA")
            values[count]=$(echo "scale=6; $(echo $line | cut -f 2 -d ':')/1" | bc -q) #valore
            (( count++ ))
            ;;
        "CAMBI DI CODA")
            values[count]=$(echo $line | cut -f 2 -d ':')
            (( values[count]++ ))
            (( count++ ))
            ;;
        "CLIENTI SERVITI" | "CHIUSURE" | "PRODOTTI BATTUTI")
           Cvalues[count]=$(echo $line | cut -f 2 -d ':')
           (( count++ ))
           ;;
        "TEMPO DI SERVIZIO")
            Cvalues[count]=$(echo "scale=3; $(echo $line | cut -f 2 -d ':')/${Cvalues[1]}" | bc -q)
            (( count++ ))
            ;;
        "TEMPI DI APERTURA")
            tmp=$(echo $line | cut -f 2 -d ':')
            sum=0;
            IFS=',' read -r -a array <<< "$tmp"
            for element in "${array[@]}"
            do
                sum=$(echo "scale=3; ($sum+$element)/1" | bc -q)
            done
            Cvalues[count]=$sum
            (( count++ ))
            ;;
    esac
    ##########################################
    if [[ $count -eq 5 && $flag -eq 0 ]]; then
        echo "|${names[0]}:${values[0]}|${names[1]}:${values[1]}|${names[2]}:${values[2]}|${names[3]}:${values[3]}|${names[4]}:${values[4]}|"
        count=0
    fi
    if [[ $count -eq 6 ]]; then
        echo "|${Cnames[0]}:${Cvalues[0]}|${Cnames[1]}:${Cvalues[1]}|${Cnames[2]}:${Cvalues[2]}|${Cnames[3]}:${Cvalues[3]}|${Cnames[4]}:${Cvalues[4]}|${Cnames[5]}:${Cvalues[5]}|"
        count=0
    fi
done
exec 3<&-
