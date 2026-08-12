# HeadPhoneFix for PSP

A PSP plugin that fixes **inverted headphone jack detection**.

**Русская версия:** [перейти к разделу на русском](#русский)

------------------------------------------------------------------------

## English

### What is HeadPhoneFix?

**HeadPhoneFix** is a system plugin for the Sony PlayStation Portable
designed to work around a hardware fault where the headphone jack
detection signal has inverted polarity.

On an affected PSP:

  Physical state            PSP detects
  ------------------------- -------------------------
  Headphones disconnected   Headphones connected
  Headphones connected      Headphones disconnected

This causes inverted audio routing: without headphones the internal
speakers are muted, while connecting headphones can enable the speakers
instead.

HeadPhoneFix corrects this detection polarity in software.

### How it works

On affected hardware the Syscon headphone state is inverted:

``` text
Physical: headphones disconnected
Syscon:   1

Physical: headphones connected
Syscon:   0
```

HeadPhoneFix dynamically locates the Syscon/HPRM structures and
headphone callback used by the currently loaded firmware environment.

The plugin:

1.  locates `sceSysconGetHPConnect`;
2.  locates `sceSysconSetHPConnectCallback`;
3.  analyzes the active Syscon callback handler;
4.  dynamically finds the headphone callback and HPRM headphone state;
5.  installs a replacement callback that inverts the detected state
    (`0 ↔ 1`);
6.  keeps the state synchronized after suspend/resume and HPRM
    reinitialization.

Because the relevant addresses are discovered at runtime, the same PRX
can be loaded separately in both **VSH (XMB)** and **GAME**
environments.

### Installation

Copy `HeadPhoneFix.prx` to:

``` text
ms0:/seplugins/HeadPhoneFix.prx
```

Add to `ms0:/seplugins/PLUGINS.TXT`:

``` text
vsh, ms0:/seplugins/HeadPhoneFix.prx, 1
game, ms0:/seplugins/HeadPhoneFix.prx, 1
```

Then fully restart the PSP.

> **Do not use the `always` runlevel.**\
> The plugin discovers the relevant system addresses when it is loaded.
> Separate VSH and GAME instances are intentional.

### Expected behavior

``` text
Headphones disconnected
→ Internal speakers

Headphones connected
→ Headphone output
```

The corrected behavior should survive suspend/resume.

### Compatibility

Developed and tested on:

-   PSP-1000 (01g)
-   TA-086 motherboard
-   System Software 6.61
-   ARK-4 cIPL

Other PSP models, motherboard revisions, firmware versions and CFWs have
not been tested. Use on untested configurations should be considered
experimental.

### Important

HeadPhoneFix targets a **specific hardware fault: inverted headphone
jack detection polarity**.

It is **not** a manual speaker/headphone switch, audio booster, volume
enhancement plugin, or universal fix for PSP audio failures.

### Recovery on ARK-4

If the PSP fails to boot after enabling the plugin:

1.  Fully power off the PSP.
2.  Hold **START**.
3.  Turn the PSP on while continuing to hold START.
4.  ARK-4 should boot with plugins disabled.
5.  Remove or disable HeadPhoneFix in `PLUGINS.TXT`.

### Technical overview

The fault occurs below the normal application audio layer: the physical
headphone-detect state reported through Syscon has reversed polarity.

HeadPhoneFix does not patch individual games or redirect audio streams.
It corrects the headphone-detection state used by the PSP system.

At startup in each environment, the plugin discovers the active Syscon
callback path and HPRM state dynamically, installs an inverted
headphone-detection callback, and keeps the state synchronized after
system reinitialization.

This avoids relying on VSH-specific HPRM addresses and allows the same
plugin binary to operate in both XMB and games.

------------------------------------------------------------------------

## Русский

### Что такое HeadPhoneFix?

**HeadPhoneFix** --- системный плагин для Sony PlayStation Portable,
предназначенный для программного обхода аппаратной неисправности, при
которой сигнал определения подключения наушников имеет обратную
полярность.

На неисправной PSP:

  Физическое состояние   PSP определяет
  ---------------------- ---------------------
  Наушники отключены     Наушники подключены
  Наушники подключены    Наушники отключены

Из-за этого маршрутизация звука работает наоборот: без подключённых
наушников встроенные динамики отключаются, а подключение наушников
может, наоборот, включать динамики.

HeadPhoneFix программно исправляет полярность сигнала определения
наушников.

### Как это работает

На неисправном железе состояние Syscon имеет обратную полярность:

``` text
Физически: наушников нет
Syscon:    1

Физически: наушники подключены
Syscon:    0
```

HeadPhoneFix динамически находит используемые текущим окружением
структуры Syscon/HPRM и callback определения наушников.

Плагин:

1.  находит `sceSysconGetHPConnect`;
2.  находит `sceSysconSetHPConnectCallback`;
3.  анализирует активный обработчик callback Syscon;
4.  динамически определяет headphone callback и состояние наушников в
    HPRM;
5.  устанавливает callback, инвертирующий состояние (`0 ↔ 1`);
6.  поддерживает правильное состояние после suspend/resume и повторной
    инициализации HPRM.

Поскольку необходимые адреса определяются во время выполнения, один и
тот же PRX можно отдельно загружать как в **VSH (XMB)**, так и в
**GAME**.

### Установка

Скопируйте `HeadPhoneFix.prx` в:

``` text
ms0:/seplugins/HeadPhoneFix.prx
```

Добавьте в `ms0:/seplugins/PLUGINS.TXT`:

``` text
vsh, ms0:/seplugins/HeadPhoneFix.prx, 1
game, ms0:/seplugins/HeadPhoneFix.prx, 1
```

После этого полностью перезагрузите PSP.

> **Не используйте runlevel `always`.**\
> Плагин определяет необходимые системные адреса при загрузке. Отдельная
> загрузка экземпляра для VSH и GAME сделана намеренно.

### Ожидаемое поведение

``` text
Наушники отключены
→ работают встроенные динамики

Наушники подключены
→ звук идёт через наушники
```

Корректное поведение должно сохраняться после перехода PSP в режим сна и
выхода из него.

### Совместимость

Разработка и тестирование проводились на:

-   PSP-1000 (01g)
-   материнская плата TA-086
-   System Software 6.61
-   ARK-4 cIPL

Другие модели PSP, ревизии материнских плат, версии прошивки и CFW пока
не тестировались. Использование на непроверенных конфигурациях следует
считать экспериментальным.

### Важно

HeadPhoneFix предназначен для обхода **конкретной аппаратной
неисправности --- инвертированной полярности сигнала определения
наушников**.

Это **не** ручной переключатель «динамики / наушники», усилитель звука,
плагин увеличения громкости или универсальное решение любых
неисправностей аудиотракта PSP.

### Восстановление на ARK-4

Если после включения плагина PSP перестала нормально загружаться:

1.  Полностью выключите PSP.
2.  Зажмите **START**.
3.  Не отпуская START, включите PSP.
4.  ARK-4 должен загрузиться с отключёнными плагинами.
5.  Удалите или отключите HeadPhoneFix в `PLUGINS.TXT`.

### Технически вкратце

Неисправность находится ниже обычного уровня игрового аудио: физическое
состояние headphone-detect, получаемое через Syscon, имеет обратную
полярность.

HeadPhoneFix не патчит отдельные игры и не перенаправляет аудиопотоки.
Вместо этого он исправляет состояние определения наушников, которым
пользуется сама система PSP.

При загрузке в каждом окружении плагин динамически определяет активный
путь Syscon callback и состояние HPRM, устанавливает callback с
инверсией headphone-detect и поддерживает правильное состояние после
повторной инициализации системы.

Благодаря этому плагин не зависит от фиксированных VSH-адресов HPRM и
один и тот же бинарный файл может работать как в XMB, так и в играх.
