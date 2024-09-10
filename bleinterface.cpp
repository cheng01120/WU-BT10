#include "bleinterface.h"
#include <QDebug>
#include <QEventLoop>

QVector<Event> events;

bool portOpened = false;
LPVM_MIDI_PORT port;

void openVirtualMIDIPort()
{
	if(portOpened) return;

	port = virtualMIDICreatePortEx2( L"WU-BT10", 0, 0, MAX_SYSEX_BUFFER, TE_VM_FLAGS_INSTANTIATE_TX_ONLY | TE_VM_FLAGS_PARSE_TX);
	if ( !port ) {
		qWarning() << "could not create port: " <<  GetLastError();
		return;
	}
	portOpened = true;
	qInfo() << "Virtual MIDI port WU-BT10 opened.";
}

void closeVirtualMIDIPort()
{
	if(!portOpened) return;
	virtualMIDIClosePort( port );
	portOpened = false;
	//qInfo() << "Virtual MIDI port WU-BT10 closed.";
}

DeviceInfo::DeviceInfo(const QBluetoothDeviceInfo &info):
    QObject(), m_device(info)
{
}

QBluetoothDeviceInfo DeviceInfo::getDevice() const
{
    return m_device;
}

QString DeviceInfo::getAddress() const
{
#ifdef Q_OS_MAC
    // workaround for Core Bluetooth:
    return m_device.deviceUuid().toString();
#else
    return m_device.address().toString();
#endif
}

void DeviceInfo::setDevice(const QBluetoothDeviceInfo &device)
{
    m_device = device;
    emit deviceChanged();
}

BLEInterface::BLEInterface(QObject *parent) : QObject(parent),
    m_currentDevice(0),
    m_control(0),
    m_service(0),
    m_readTimer(0),
		m_noteOnTimer(0),
    m_connected(false),
    m_currentService(0)
{
    m_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    connect(m_deviceDiscoveryAgent, SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)),
            this, SLOT(addDevice(const QBluetoothDeviceInfo&)));
    connect(m_deviceDiscoveryAgent, SIGNAL(errorOccurred(QBluetoothDeviceDiscoveryAgent::Error)),
            this, SLOT(onDeviceScanError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(m_deviceDiscoveryAgent, SIGNAL(finished()), this, SLOT(onScanFinished()));
}

BLEInterface::~BLEInterface()
{
    qDeleteAll(m_devices);
    m_devices.clear();
		closeVirtualMIDIPort();
}

void BLEInterface::scanDevices()
{
    m_devicesNames.clear();
    qDeleteAll(m_devices);
    m_devices.clear();
    emit devicesNamesChanged(m_devicesNames);
    m_deviceDiscoveryAgent->start();
    qInfo("Scanning for devices...");
}

void BLEInterface::read(){
    if(m_service && m_readCharacteristic.isValid())
        m_service->readCharacteristic(m_readCharacteristic);
}

void BLEInterface::waitForWrite() {
    QEventLoop loop;
    connect(m_service, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)),
            &loop, SLOT(quit()));
    loop.exec();
}

void BLEInterface::write(const QByteArray &data)
{
    qDebug() << "BLEInterface::write: " << data;
    if(m_service && m_writeCharacteristic.isValid()){
        if(data.length() > CHUNK_SIZE){
            int sentBytes = 0;
            while (sentBytes < data.length()) {
                m_service->writeCharacteristic(m_writeCharacteristic,
                                               data.mid(sentBytes, CHUNK_SIZE),
                                               m_writeMode);
                sentBytes += CHUNK_SIZE;
                if(m_writeMode == QLowEnergyService::WriteWithResponse){
                    waitForWrite();
                    if(m_service->error() != QLowEnergyService::NoError)
                        return;
                }
            }

        }
        else
            m_service->writeCharacteristic(m_writeCharacteristic, data, m_writeMode);
    }
}

void BLEInterface::addDevice(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
			if(device.name().indexOf("WU-BT10") == 0) {
        qWarning() << "Discovered LE Device name: " << device.name() << " Address: "
                   << device.address().toString();
        m_devicesNames.append(device.name());
        DeviceInfo *dev = new DeviceInfo(device);
        m_devices.append(dev);
    		m_deviceDiscoveryAgent->stop();
        emit devicesNamesChanged(m_devicesNames);
        qInfo("Low Energy device found. Scanning for service...");
        emit statusInfoChanged("Done.", true);
				connectCurrentDevice();
			}
    }
}

void BLEInterface::onScanFinished()
{
    if (m_devicesNames.size() == 0) qInfo("No Low Energy devices found.");
}

void BLEInterface::onDeviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
        qInfo("The Bluetooth adaptor is powered off, power it on before doing discovery.");
    else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
        qInfo("Writing or reading from the device resulted in an error.");
    else
        qInfo("An unknown error has occurred.");
}



void BLEInterface::connectCurrentDevice()
{
    if(m_devices.isEmpty())
        return;

    if (m_control) {
        m_control->disconnectFromDevice();
        delete m_control;
        m_control = 0;

    }
    m_control = QLowEnergyController::createCentral(m_devices[ m_currentDevice]->getDevice(), this);
    connect(m_control, SIGNAL(serviceDiscovered(QBluetoothUuid)),
            this, SLOT(onServiceDiscovered(QBluetoothUuid)));
    connect(m_control, SIGNAL(discoveryFinished()),
            this, SLOT(onServiceScanDone()));
    connect(m_control, SIGNAL(errorOccurred(QLowEnergyController::Error)),
            this, SLOT(onControllerError(QLowEnergyController::Error)));
    connect(m_control, SIGNAL(connected()),
            this, SLOT(onDeviceConnected()));
    connect(m_control, SIGNAL(disconnected()),
            this, SLOT(onDeviceDisconnected()));

    m_control->connectToDevice();
}

void BLEInterface::onDeviceConnected()
{
    m_servicesUuid.clear();
    m_services.clear();
    setCurrentService(-1);
    emit servicesChanged(m_services);
    m_control->discoverServices();
}

void BLEInterface::onDeviceDisconnected()
{
    update_connected(false);
    emit statusInfoChanged("Service disconnected", false);
    qWarning() << "Remote device disconnected";
}

void BLEInterface::onServiceDiscovered(const QBluetoothUuid &gatt)
{
    Q_UNUSED(gatt)
    qInfo("Service discovered. Waiting for service scan to be done...");
}

void BLEInterface::onServiceScanDone()
{
    m_servicesUuid = m_control->services();
    if(m_servicesUuid.isEmpty())
        qInfo("Can't find any services.");
    else{
			  std::reverse(m_servicesUuid.begin(), m_servicesUuid.end());
        m_services.clear();
        foreach (auto uuid, m_servicesUuid)
            m_services.append(uuid.toString());
        emit servicesChanged(m_services);
        m_currentService = -1;// to force call update_currentService(once)
        setCurrentService(0);
        qInfo("All services discovered.");

				openVirtualMIDIPort();
				m_elapsed.start();
				if(!m_noteOnTimer){
						m_noteOnTimer = new QTimer(this);
						connect(m_noteOnTimer, &QTimer::timeout, this, &BLEInterface::checkNoteOnTooLong);
						m_noteOnTimer->start(MAX_NOTE_ON_MS);
				}
    }
}


void BLEInterface::disconnectDevice()
{
    m_readTimer->deleteLater();
    m_readTimer = NULL;

		m_noteOnTimer->deleteLater();
		m_noteOnTimer = NULL;

		closeVirtualMIDIPort();

    if (m_devices.isEmpty()) {
        return;
    }

    //disable notifications
    if (m_notificationDesc.isValid() && m_service) {
        m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0000"));
    } else {
        m_control->disconnectFromDevice();
        delete m_service;
        m_service = 0;
    }
}

void BLEInterface::onControllerError(QLowEnergyController::Error error)
{
    qInfo("Cannot connect to remote device.");
    qWarning() << "Controller Error:" << error;
}

void BLEInterface::onCharacteristicChanged(const QLowEnergyCharacteristic &c,
                                           const QByteArray &value)
{
    Q_UNUSED(c)
    //emit dataReceived(value);
		parseBLEPacket(value);
}

void BLEInterface::parseBLEPacket(const QByteArray& value)
{
	u8 ss = value.size() -1;
	if(ss & 0x03)  {
		qWarning() << "Unsupported message : " << value.toHex(':').toUpper();
		return;
	}

	u8 high = value[0] & 0b00111111;
	u8 low  = value[1] & 0b01111111;
	u16 t0 = (high << 7) | low;
	for(int i = 1; i < value.size(); i += 4) {
		u8 l = value[i] & 0b01111111;
		if( l < low) high += 1;
		u16 t1 = (high << 7) | l;
		low = l;

		u8 st      = value[i+1] & 0xf0;
		u8 channel = value[i+1] & 0x0f;
		u8 key     = value[i+2];
		u8 vel     = value[i+3];

		// 发送note on之前先检查是否没有note off的key
		if((st == 0x90) && vel) { // note on 
			for(int i = 0; i < events.size(); i++) {
				if((events[i].key == key) && (events[i].channel == channel)) {
					qWarning() << "Repeat Note on for key: " << events[i].key;
					sendNoteOff(key, channel);
					events.removeAt(i);
					break;
				}
			}
			events.push_back(Event(m_elapsed.elapsed(), key, channel)); // keep track of note on'd keys.
		}

		if( (st == 0x80) || ( (st == 0x90) && !vel))  { // note off
			for(int i = 0; i < events.size(); i++) {
				if(events[i].key == key && events[i].channel == channel) {
					events.removeAt(i);
					break;
				}
			}
			//qInfo() << "No. of keys is staill on: " << events.size();
		}

		Sleep(t1 - t0);
		QByteArray message(value.data() + i + 1, 3);
	  if (!virtualMIDISendData(port, (u8 *)message.data(), 3)) {
			qWarning() << "Unable to send midi message: " << message.toHex(':').toUpper();
		}
	}
}

void BLEInterface::onCharacteristicWrite(const QLowEnergyCharacteristic &c,
                                          const QByteArray &value)
{
    Q_UNUSED(c)
    qDebug() << "Characteristic Written: " << value;
}

void BLEInterface::update_currentService(int indx)
{
    delete m_service;
    m_service = 0;

    if (indx >= 0 && m_servicesUuid.count() > indx) {
        m_service = m_control->createServiceObject(
                    m_servicesUuid.at(indx), this);
    }

    if (!m_service) {
        qInfo("Service not found.");
        return;
    }

    connect(m_service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)),
            this, SLOT(onServiceStateChanged(QLowEnergyService::ServiceState)));
    connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),
            this, SLOT(onCharacteristicChanged(QLowEnergyCharacteristic,QByteArray)));
    connect(m_service, SIGNAL(characteristicRead(QLowEnergyCharacteristic,QByteArray)),
            this, SLOT(onCharacteristicRead(QLowEnergyCharacteristic,QByteArray)));
    connect(m_service, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)),
            this, SLOT(onCharacteristicWrite(QLowEnergyCharacteristic,QByteArray)));
    connect(m_service, SIGNAL(errorOccurred(QLowEnergyService::ServiceError)),
            this, SLOT(serviceError(QLowEnergyService::ServiceError)));

    if(m_service->state() == QLowEnergyService::DiscoveryRequired) {
        qInfo("Connecting to service...");
        m_service->discoverDetails();
    }
    else
        searchCharacteristic();
}
void BLEInterface::onCharacteristicRead(const QLowEnergyCharacteristic &c,
                                        const QByteArray &value){
    Q_UNUSED(c);
		Q_UNUSED(value);
}

void BLEInterface::searchCharacteristic(){
    if(m_service){
        foreach (QLowEnergyCharacteristic c, m_service->characteristics()) {
            if(c.isValid()){
                if (c.properties() & QLowEnergyCharacteristic::WriteNoResponse ||
                    c.properties() & QLowEnergyCharacteristic::Write) {
                    m_writeCharacteristic = c;
                    update_connected(true);
                    if(c.properties() & QLowEnergyCharacteristic::WriteNoResponse)
                        m_writeMode = QLowEnergyService::WriteWithoutResponse;
                    else
                        m_writeMode = QLowEnergyService::WriteWithResponse;

                }
                if (c.properties() & QLowEnergyCharacteristic::Read) {
                    m_readCharacteristic = c;
                    if(!m_readTimer){
                        m_readTimer = new QTimer(this);
                        connect(m_readTimer, &QTimer::timeout, this, &BLEInterface::read);
                        m_readTimer->start(READ_INTERVAL_MS);
                    }
                }
                m_notificationDesc = c.descriptor(
                            QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
                if (m_notificationDesc.isValid()) {
                    m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
                }
            }
        }
    }
}

void BLEInterface::onServiceStateChanged(QLowEnergyService::ServiceState s)
{
    //qDebug() << "serviceStateChanged, state: " << s;
    if (s == QLowEnergyService::ServiceDiscovered) {
        searchCharacteristic();
    }
}
void BLEInterface::serviceError(QLowEnergyService::ServiceError e)
{
    qWarning() << "Service error:" << e;
}

void BLEInterface::checkNoteOnTooLong()
{
	if(events.isEmpty()) return;

	unsigned now = m_elapsed.elapsed();
	int i = 0;
	while(i < events.size()) {
		if(now - events[i].ts > MAX_NOTE_ON_MS) {
			qWarning() << "Key " << events[i].key << " note on too long.";
			sendNoteOff(events[i].key, events[i].channel);
			events.removeAt(i);
		}
		else {
			i++;
		}
	}
}

void BLEInterface::sendNoteOff(u8 key, u8 channel) {
	u8 a[3];
	a[0] = 0x80 | channel;
	a[1] = key;
	a[2] = 0x80;
	virtualMIDISendData(port, a, 3);
}
