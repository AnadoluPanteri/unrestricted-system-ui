#include "sysuid.h"
#include "batterybusinesslogic.h"
#include <qmsystem/qmled.h>

#include <DuiLocale>

#include <QTimer>

/* TODO List

   1) Create notification for RechargeBatteryText (have to get signal to know when the device is turned off coz of the
      too low battery)   
   2) What are correct animation rates when charging with USB / Wall?
   3) If USB 100 mA is used, do we show animation at all? In Fremantle not.
   4) Connect sounds with notifications
   5) reminder notifications

*/

namespace {
    const QString PSMActivateText = trid("qtn_ener_aps", "Activate power save now");
    const QString PSMDeactivateText = trid("qtn_ener_dps", "Deactivate power save now");
    const QString ChargingText = trid("qtn_ener_char", "Charging");
    const QString ChargingNotStartedText = trid("qtn_ener_repcharger", "Charging not started. Replace charger.");
    const QString ChargingCompleteText = trid("qtn_ener_charcomp", "Charging complete");
    const QString DisconnectChargerText = trid("qtn_ener_remcha", "Disconnect charger from power supply to save energy");
    const QString LowBatteryText = trid("qtn_ener_lowbatt", "Low battery");
    const QString EnterPSMText = trid("qtn_ener_ent_psnote", "Entering power save mode");
    const QString ExitPSMText = trid("qtn_ener_exit_psnote", "Exiting power save mode");
    const QString RechargeBatteryText = trid("qtn_ener_rebatt", "Recharge battery");
    const int LowBatteryActiveInterval = 30; //30 mins
    const int LowBatteryInactiveInterval = 120; //120 mins
    const int ChargingAnimationRateUSB = 800; // 800 ms
    const int ChargingAnimationRateWall = 400; // 400 ms
}

LowBatteryNotifier::LowBatteryNotifier(Notifier *uiNotif, LockScreenBusinessLogic::ScreenLockPhase phase, QObject* parent) :
        QObject(parent),
        uiNotif(uiNotif),
        minsToElapse(LowBatteryActiveInterval),
        minsElapsed(0),
        showReminder(false),
        timer(new QTimer(this))
{
    lockScreenPhaseChanged(phase);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateElapsedTime()));    
}

LowBatteryNotifier::~LowBatteryNotifier()
{    
}

void LowBatteryNotifier::start()
{
    uiNotif->showNotification(LowBatteryText);
    timer->start(60 * 1000); //signal every minute
}

void LowBatteryNotifier::stop()
{
    timer->stop();
}

void LowBatteryNotifier::lockScreenPhaseChanged(LockScreenBusinessLogic::ScreenLockPhase phase)
{
    switch(phase) {
        case LockScreenBusinessLogic::Off:
        case LockScreenBusinessLogic::On:
            if(showReminder) {                 
                 uiNotif->showNotification(LowBatteryText);
                 showReminder = false;
            }
            minsToElapse = LowBatteryActiveInterval;
            break;
        case LockScreenBusinessLogic::Sleep:
            minsToElapse = LowBatteryInactiveInterval;
            break;
    }

}

void LowBatteryNotifier::updateElapsedTime()
{
    ++minsElapsed;
    if(minsElapsed >= minsToElapse) {        
        uiNotif->showNotification(LowBatteryText);
        minsElapsed = 0;
        if(minsToElapse == LowBatteryInactiveInterval) // currently in sleep mode
            showReminder = true;
    }
}

BatteryBusinessLogic::BatteryBusinessLogic(SystemUIGConf *systemUIGConf, LockScreenBusinessLogic *lockScreenLogic, QObject* parent) :
        QObject(parent),
        systemUIGConf(systemUIGConf),
        lockScreenLogic(lockScreenLogic),
        battery(new QmBattery()),
        deviceMode(new QmDeviceMode()),
        uiNotif(Sysuid::notifier()),
        lowBatteryNotifier(NULL)
{

    /* init the PSM thresholds */
    PSMThresholds << QString("5") << QString("10") << QString("15") << QString("25") << QString("35")
            << QString("50") << QString("60") << QString("75") << QString("85") << QString("100");

    /* check if gconfvalues need to initialized */
    initSystemUIGConfKeys();

    /* connect to QmSystem signals */
    connect(battery, SIGNAL(batteryLevelChanged(Maemo::QmBattery::Level)),
            this, SLOT(batteryLevelChanged(Maemo::QmBattery::Level)));
    connect(battery, SIGNAL(batteryStatusChanged(Maemo::QmBattery::State)),
            this, SLOT(batteryStatusChanged(Maemo::QmBattery::State)));
    connect(battery, SIGNAL(batteryEnergyLevelChanged(int)),
            this, SLOT(batteryEnergyLevelChanged(int)));
    connect(battery, SIGNAL(chargerEvent(Maemo::QmBattery::ChargerEvent)),
            this, SLOT(batteryChargerEvent(Maemo::QmBattery::ChargerEvent)));
    connect(deviceMode, SIGNAL(devicePSMStateChanged(Maemo::QmDeviceMode::PSMState)),
            this, SLOT(devicePSMStateChanged(Maemo::QmDeviceMode::PSMState)));

    initBattery();
}

BatteryBusinessLogic::~BatteryBusinessLogic()
{        
    delete battery;
    battery = NULL;
    delete deviceMode;
    deviceMode = NULL;
    uiNotif = NULL; // not owned!
}


void BatteryBusinessLogic::initSystemUIGConfKeys()
{
    if(systemUIGConf->keyCount(SystemUIGConf::Battery) < 2) {
        /* GConf keys have not yet been set. */        
        systemUIGConf->setValue(SystemUIGConf::BatteryPSMAutoKey, QVariant(true));       
        systemUIGConf->setValue(SystemUIGConf::BatteryPSMThresholdKey, QVariant(1));
    }    
}


// This method should be called also when the device is returned from sleep mode
void BatteryBusinessLogic::initBattery()
{
    //init the charging status
    batteryStatusChanged(battery->getState());

    //init the battery level
    batteryLevelChanged(battery->getLevel());
}

int BatteryBusinessLogic::batteryBarValue(int percentage)
{   
    if(percentage == -1)
        percentage = battery->getBatteryEnergyLevel();

    int index = 0;
    if(percentage >= 88)
        index = PSMThresholds.indexOf("100");
    else if(percentage < 88 && percentage >= 75)
        index = PSMThresholds.indexOf("85");
    else if(percentage < 75 && percentage >= 63)
        index = PSMThresholds.indexOf("75");
    else if(percentage < 63 && percentage >= 50)
        index = PSMThresholds.indexOf("60");
    else if(percentage < 50 && percentage >= 38)
        index = PSMThresholds.indexOf("50");
    else if(percentage < 38 && percentage >= 25)
        index = PSMThresholds.indexOf("35");
    else if(percentage < 25 && percentage >= 13)
        index = PSMThresholds.indexOf("25");
    else if(percentage < 13 && percentage >= 10)
        index = PSMThresholds.indexOf("15");
    else if(percentage < 10 && percentage >= 5)
        index = PSMThresholds.indexOf("10");
    else if(percentage < 5)
        index = PSMThresholds.indexOf("5");

    return index;
}

void BatteryBusinessLogic::batteryStatusChanged(Maemo::QmBattery::State state)
{
    //////qDebug() << "BatteryBusinessLogic::batteryStatusChanged(" << state << ")";
    switch(state) {        
        case QmBattery::StateCharging:
            ////qDebug() << "Charging";
            emit batteryCharging(animationRate());
            utiliseLED(true, QString("PatternBatteryCharging"));            
            uiNotif->showNotification(ChargingText);
            if(lowBatteryNotifier != NULL) {
                lowBatteryNotifier->stop();
                delete lowBatteryNotifier;
                lowBatteryNotifier = NULL;
            }
            break;
        case QmBattery::StateNotCharging:
            ////qDebug() << "Not charging";
            emit batteryNotCharging();
            utiliseLED(false, QString("PatternBatteryCharging"));
            break;
        case QmBattery::StateChargingFailed:
            ////qDebug() << "Charging not started";
            emit batteryNotCharging();
            utiliseLED(false, QString("PatternBatteryCharging"));
            uiNotif->showNotification(ChargingNotStartedText);
            break;
        default:
            break;
    }
    emit remainingTimeValuesChanged(remainingTimeValues());
}

void BatteryBusinessLogic::batteryLevelChanged(Maemo::QmBattery::Level level)
{
    ////qDebug() << "BatteryBusinessLogic::batteryLevelChanged(" << level << ")";

    switch(level) {
        case QmBattery::LevelFull:            
            uiNotif->showNotification(ChargingCompleteText);
            utiliseLED(true, QString("PatternBatteryFull"));
            break;
        case QmBattery::LevelLow:            
            if(!battery->getState() == QmBattery::StateCharging) {                
                if(lowBatteryNotifier == NULL) {
                    lowBatteryNotifier = new LowBatteryNotifier(uiNotif, lockScreenLogic->screenLockPhase());
                    connect(lockScreenLogic, SIGNAL(screenLockPhaseChanged(LockScreenBusinessLogic::ScreenLockPhase)),
                            lowBatteryNotifier, SLOT(lockScreenPhaseChanged(LockScreenBusinessLogic::ScreenLockPhase)));
                }
                lowBatteryNotifier->start();
            }
            break;
        default:
            break;
   }
}

void BatteryBusinessLogic::batteryEnergyLevelChanged(int percentage)
{
    ////qDebug() << "BatteryBusinessLogic::batteryEnergyLevelChanged(" << percentage << ")";
    emit batteryBarValueChanged(batteryBarValue(percentage));
    checkPSMThreshold();
}

void BatteryBusinessLogic::batteryChargerEvent(Maemo::QmBattery::ChargerEvent event)
{
    switch(event) {
        case QmBattery::ChargerDisconnected:
            uiNotif->showNotification(DisconnectChargerText);
            break;
        default:
            break;
    }
}

void BatteryBusinessLogic::devicePSMStateChanged(Maemo::QmDeviceMode::PSMState PSMState)
{    
    ////qDebug() << "BatteryBusinessLogic::devicePSMStateChanged(" << PSMState << ")";
    if(PSMState == QmDeviceMode::PSMStateOff) {
        uiNotif->showNotification(ExitPSMText);
        emit PSMValueChanged(PSMActivateText);
    }
    else if(PSMState == QmDeviceMode::PSMStateOn) {
        uiNotif->showNotification(EnterPSMText);
        emit PSMValueChanged(PSMDeactivateText);
    }
    emit remainingTimeValuesChanged(remainingTimeValues());
}

void BatteryBusinessLogic::checkPSMThreshold()
{
    ////qDebug() << "BatteryBusinessLogic::checkPSMThreshold()";
    if(systemUIGConf->value(SystemUIGConf::BatteryPSMAutoKey).toBool()) { // we only handle this if the auto PSM is on        

        if(batteryBarValue(battery->getBatteryEnergyLevel()) <= systemUIGConf->value(SystemUIGConf::BatteryPSMThresholdKey).toInt()) {
            if(deviceMode->getPSMState() == QmDeviceMode::PSMStateOff)
                deviceMode->setPSMState(QmDeviceMode::PSMStateOn);
        }
        else {
            if(deviceMode->getPSMState() == QmDeviceMode::PSMStateOn)
                deviceMode->setPSMState(QmDeviceMode::PSMStateOff);
        }
    }
}

QString BatteryBusinessLogic::PSMValue()
{
    ////qDebug() << "BatteryBusinessLogic::PSMValue()";
    return ( deviceMode->getPSMState() == QmDeviceMode::PSMStateOn ? PSMDeactivateText : PSMActivateText );
}

void BatteryBusinessLogic::togglePSM(const QString &value)
{
    ////qDebug() << "BatteryBusinessLogic::togglePSM(" << value << ")";

    if(value == PSMActivateText)
        deviceMode->setPSMState(QmDeviceMode::PSMStateOn); //turn on the PSM
    else
        deviceMode->setPSMState(QmDeviceMode::PSMStateOff); //turn off the PSM

    //when ever we toggle PSM manually, we turn off the automatic PSM
    systemUIGConf->setValue(SystemUIGConf::BatteryPSMAutoKey, QVariant(false));
    emit PSMAutoValueChanged(false);    
}

void BatteryBusinessLogic::togglePSMAuto(bool toggle)
{
    ////qDebug() << "BatteryBusinessLogic::togglePSMAuto(" << toggle << ")";
    systemUIGConf->setValue(SystemUIGConf::BatteryPSMAutoKey, QVariant(toggle));
    if(toggle) // if we turn on the Auto PSM, we must check the threshold
        checkPSMThreshold();
    else { // if we turn off the Auto PSM, we must disable the PSM in all cases
        if(deviceMode->getPSMState() == QmDeviceMode::PSMStateOn)
            deviceMode->setPSMState(QmDeviceMode::PSMStateOff);     
    }

}

QStringList BatteryBusinessLogic::remainingTimeValues()
{          
    ////qDebug() << "BatteryBusinessLogic::remainingTimeValues()";

    QStringList values;
    if(battery->getState() == QmBattery::StateCharging)
        values << ChargingText << ChargingText;
    else
        values << QString("%1").arg(battery->remainingTalkTime() / 60) << QString("%1").arg(battery->remainingTime() / 60);
    return values;
}

void BatteryBusinessLogic::utiliseLED(bool activate, const QString &pattern)
{
    QmLED led;
    if(activate)
        led.activate(pattern);
    else
        led.deactivate(pattern);
}

void BatteryBusinessLogic::setPSMThreshold(const QString &threshold)
{
    ////qDebug() << "BatteryBusinessLogic::setPSMthreshold(" << threshold << ")";
    systemUIGConf->setValue(SystemUIGConf::BatteryPSMThresholdKey, QVariant(PSMThresholds.indexOf(threshold)));
    checkPSMThreshold();
}

QVariant BatteryBusinessLogic::GConfItemValue(SystemUIGConf::GConfKey key)
{
    return systemUIGConf->value(key);
}

QStringList BatteryBusinessLogic::PSMThresholdValues()
{
    //TODO: replace hardcoded values with real ones when they are available
    QStringList values;
    for(int i=0; i<5; ++i)
        values << PSMThresholds.at(i);
    return values;
}

QString BatteryBusinessLogic::PSMThresholdValue()
{
    return PSMThresholds.at(systemUIGConf->value(SystemUIGConf::BatteryPSMThresholdKey).toInt());
}

void BatteryBusinessLogic::batteryStatus()
{
    switch(battery->getState()) {
        case QmBattery::StateCharging:
            emit batteryCharging(animationRate());
            break;
        case QmBattery::StateNotCharging:
        case QmBattery::StateChargingFailed:
            emit batteryNotCharging();
            break;
        default:
            break;
    }
}

int BatteryBusinessLogic::animationRate()
{
    int rate = -1;
    switch(battery->getChargerType()) {
        case QmBattery::USB_500mA:
            rate = ChargingAnimationRateUSB;
            break;
        case QmBattery::Wall:
            rate = ChargingAnimationRateWall;
            break;
        default:
            break;
    }
    return rate;
}
