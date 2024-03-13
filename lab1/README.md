# Лабораторная работа 1

**Название:** "Разработка драйверов символьных устройств"

**Цель работы:**

Написать драйвер символьного устройства

## Описание функциональности драйвера

1. Драйвер должен создавать символьное устройство /dev/io_lab

2. Данный драйвер должен подсчитывать количество запятых на операции write, с момента загрузки модуля, и выводить их на read

## Инструкция по сборке

```bash
make
make install
sudo chmod 777 /dev/io_lab
операции
make remove
```

## Инструкция пользователя

`sudo cat /dev/io_lab` - открыть символьное устройство

`sudo echo "smth" >> /dev/io_lab` - вставить в символьное устройство

`make clean` - удалить файлы компиляции из директории

## Примеры использования

```shell
make install
sudo chmod 777 /dev/io_lab
sudo cat /dev/io_lab
0
echo "hello, there" > /dev/io_lab
cat /dev/io_lab
1
echo "bye,,,," > /dev/io_lab
sudo cat /dev/io_lab
5
sudo echo "bye" > /dev/io_lab
sudo cat /dev/io_lab
5
```

![Скрин работы](./images/work.png)
