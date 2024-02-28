# Лабораторная работа 1

**Название:** "Разработка драйверов символьных устройств"

**Цель работы:** 
Написать драйвер символьного устройства

## Описание функциональности драйвера
1.1 Драйвер должен создавать символьное устройство /dev/io_lab
1.2 Данный драйвер должен подсчитывать количество запятых на операции write, с момента загрузки модуля, и выводить их на read

## Инструкция по сборке
make
make install
sudo chmod 777 /dev/io_lab
операции
make remove
...

## Инструкция пользователя
sudo cat /dev/io_lab - открыть символьное устройство
sudo echo "smth" >> /dev/io_lab - вставить в символьное устройство

## Примеры использования
make install
sudo cat /dev/io_lab

<font color="green">0</font>

echo "hello, there" > /dev/io_lab
cat /dev/io_lab

<font color="green">1</font>

echo "bye,,,," > /dev/io_lab
sudo cat /dev/io_lab

<font color="green">5</font>

sudo echo "bye" > /dev/io_lab
sudo cat /dev/io_lab

<font color="green">5</font>

![Скрин работы](./images/work.png)



...
