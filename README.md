# PSP-HPInvert-plugin
# JackSenseFix for PSP

A PSP plugin that fixes inverted headphone jack detection.

Русская версия находится ниже: [Русский](#русский)

---

## English

### What is JackSenseFix?

**JackSenseFix** is a system plugin for the Sony PlayStation Portable designed to work around a hardware fault where the headphone jack detection signal is inverted.

On an affected PSP, the system behaves like this:

| Physical state | PSP detects |
| --- | --- |
| Headphones disconnected | Headphones connected |
| Headphones connected | Headphones disconnected |

This results in inverted audio routing:

- without headphones, the internal speakers are muted;
- when headphones are plugged in, the PSP may enable the internal speakers instead;
- the headphone output and speakers effectively behave backwards.

JackSenseFix corrects the detection polarity in software.

### How it works

The PSP Syscon reports the headphone connection state to the system.

On the affected hardware, this state has inverted polarity:

Physical: headphones disconnected
Syscon:   1

Physical: headphones connected
Syscon:   0

JackSenseFix dynamically locates the relevant Syscon/HPRM structures and headphone callback used by the currently loaded firmware environment.

Instead of relying on fixed HPRM addresses, the plugin:

locates sceSysconGetHPConnect;
locates sceSysconSetHPConnectCallback;
analyzes the currently loaded Syscon callback handler;
finds the active headphone callback and HPRM state dynamically;
installs a small replacement callback that inverts the headphone detection state (0 ↔ 1);
monitors the state so the fix is restored after suspend/resume or HPRM reinitialization.

This allows the same plugin binary to work both in the XMB and in games without relying on VSH-specific HPRM addresses.

Installation

Copy:

jacksensefix.prx

to:

ms0:/seplugins/jacksensefix.prx

Add the following lines to:

ms0:/seplugins/PLUGINS.TXT
vsh, ms0:/seplugins/jacksensefix.prx, 1
game, ms0:/seplugins/jacksensefix.prx, 1

Then fully restart the PSP.

Do not load the plugin using the always runlevel. The plugin performs environment discovery when it is loaded, so separate VSH and GAME instances are intentional.

Expected behavior

With JackSenseFix enabled:

No headphones
→ internal speakers

Headphones connected
→ headphone output

The behavior should remain correct after suspend/resume.

Compatibility

Developed and tested on:

PSP-1000 (01g)
TA-086 motherboard
System Software 6.61
ARK-4 cIPL

Other PSP models, motherboard revisions, firmware versions and CFWs have not been tested.

The plugin performs several sanity checks before applying the patch, but use on untested configurations is still experimental.

Important

This plugin is intended to work around a specific hardware fault where the headphone detection polarity is reversed.

It is not:

a manual speaker/headphone switch;
an audio booster;
a volume plugin;
a replacement for a physically damaged audio output stage.

If your PSP has different audio symptoms, this plugin may not help.

Recovery

If the PSP fails to boot after enabling the plugin on ARK-4:

Fully power off the PSP.
Hold START.
Turn the PSP on while continuing to hold START.
ARK-4 should boot with plugins disabled.
Remove or disable JackSenseFix in PLUGINS.TXT.
Русский
Что такое JackSenseFix?

JackSenseFix — системный плагин для Sony PlayStation Portable, предназначенный для программного обхода аппаратной неисправности, при которой сигнал определения наушников работает с обратной полярностью.

На неисправной PSP система определяет состояние разъёма наоборот:

Физическое состояние	PSP определяет
Наушники отключены	Наушники подключены
Наушники подключены	Наушники отключены

Из-за этого маршрутизация звука также работает наоборот:

без подключённых наушников встроенные динамики отключаются;
при подключении наушников PSP может включать встроенные динамики;
динамики и выход на наушники фактически работают в обратном режиме.

JackSenseFix программно исправляет полярность сигнала определения наушников.

Как это работает

За определение подключения наушников в PSP отвечает в том числе Syscon.

На неисправном железе состояние имеет обратную полярность:

Физически: наушников нет
Syscon:    1

Физически: наушники подключены
Syscon:    0

JackSenseFix динамически находит используемые текущей системой структуры Syscon/HPRM и callback определения наушников.

Вместо использования жёстко заданных адресов HPRM плагин:

находит sceSysconGetHPConnect;
находит sceSysconSetHPConnectCallback;
анализирует загруженный обработчик callback Syscon;
динамически определяет текущий headphone callback и адрес состояния HPRM;
устанавливает небольшой callback, который инвертирует состояние (0 ↔ 1);
контролирует состояние и восстанавливает корректное значение после suspend/resume или повторной инициализации HPRM.

Благодаря динамическому поиску один и тот же бинарный файл плагина может использоваться как в XMB, так и в играх без привязки к фиксированным VSH-адресам HPRM.

Установка

Скопируйте:

jacksensefix.prx

в:

ms0:/seplugins/jacksensefix.prx

Добавьте в:

ms0:/seplugins/PLUGINS.TXT

две строки:

vsh, ms0:/seplugins/jacksensefix.prx, 1
game, ms0:/seplugins/jacksensefix.prx, 1

После этого полностью перезагрузите PSP.

Не используйте для этого плагина режим always. Плагин определяет необходимые системные адреса при загрузке, поэтому отдельная загрузка для VSH и GAME сделана намеренно.

Ожидаемое поведение

После установки JackSenseFix:

Наушники отключены
→ работают встроенные динамики

Наушники подключены
→ звук идёт через наушники

Корректное поведение должно сохраняться после перехода PSP в режим сна и выхода из него.

Совместимость

Разработка и тестирование проводились на:

PSP-1000 (01g)
материнская плата TA-086
System Software 6.61
ARK-4 cIPL

Другие модели PSP, ревизии материнских плат, версии прошивки и CFW пока не тестировались.

Перед применением патча плагин выполняет несколько проверок ожидаемой структуры системного кода, однако использование на непроверенных конфигурациях всё равно следует считать экспериментальным.

Важно

Плагин предназначен для обхода конкретной аппаратной неисправности — инвертированной полярности headphone detect.

Это не:

ручной переключатель «динамики / наушники»;
усилитель звука;
плагин увеличения громкости;
решение любых неисправностей аудиотракта PSP.

Если у вашей PSP другие симптомы, JackSenseFix может не помочь.

Восстановление

Если после включения плагина PSP с ARK-4 перестала нормально загружаться:

Полностью выключите PSP.
Зажмите START.
Не отпуская START, включите PSP.
ARK-4 должен загрузиться с отключёнными плагинами.
Удалите или отключите JackSenseFix в PLUGINS.TXT.
