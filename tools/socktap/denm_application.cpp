#include "denm_application.hpp"
#include <vanetza/btp/ports.hpp>
#include <vanetza/asn1/denm.hpp>
#include <vanetza/asn1/packet_visitor.hpp>
#include <vanetza/facilities/denm_functions.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
//hard
#include <boost/format.hpp>

// This is a very simple CA application sending DENMs at a fixed rate.

using namespace vanetza;
using namespace vanetza::facilities;
using namespace std::chrono;

DenmApplication::DenmApplication(PositionProvider& positioning, Runtime& rt) :
    positioning_(positioning), runtime_(rt), denm_interval_(seconds(1))
{
    schedule_timer();
}

void DenmApplication::set_interval(Clock::duration interval)
{
    denm_interval_ = interval;
    runtime_.cancel(this);
    schedule_timer();
}

void DenmApplication::print_generated_message(bool flag)
{
    print_tx_msg_ = flag;
}

void DenmApplication::print_received_message(bool flag)
{
    print_rx_msg_ = flag;
}

DenmApplication::PortType DenmApplication::port()
{
    return btp::ports::DENM;
}

void DenmApplication::indicate(const DataIndication& indication, UpPacketPtr packet)
{
    asn1::PacketVisitor<asn1::Denm> visitor;
    std::shared_ptr<const asn1::Denm> denm = boost::apply_visitor(visitor, *packet);

    //hard
    std::cout<<"HELLO in indicate\n";
    std::cout << "DENM application received a packet with " << (denm ? "decodable" : "broken") << " content" << std::endl;
    if (denm && print_rx_msg_) {
        std::cout << "Received DENM contains\n";
        print_denm(std::cout, *denm, "  ", 1);
    }
}

void DenmApplication::schedule_timer()
{
    runtime_.schedule(denm_interval_, std::bind(&DenmApplication::on_timer, this, std::placeholders::_1), this);
}

void DenmApplication::on_timer(Clock::time_point)
{
    schedule_timer();
    vanetza::asn1::Denm message;
    //hardcoded
    //std::cout<<message<<"\n";
    char messagereceived[] = "\x01\x01\x00\x01\x0e";
    ItsPduHeader_t& header = message->header;
    std::cout<<header.protocolVersion<<"is the Header\n";


    header.protocolVersion = messagereceived[0];
    
    header.messageID = ItsPduHeader__messageID_denm;
    header.stationID = int('S'); // some dummy value

    const auto time_now = duration_cast<milliseconds>(runtime_.now().time_since_epoch());
    long gen_delta_time = time_now.count();

    DecentralizedEnvironmentalNotificationMessage_t& denm = message->denm;
    //actionID
    //const TimestampIts_t test;
    //test.buf[]
    //unsigned char testdetect {'5'};
    denm.management.actionID.originatingStationID = int('S');
    denm.management.actionID.sequenceNumber = 1;
    //detectiontime
    //TimestampIts_t testdetectptr;
    //testdetectptr.buf = &testdetect;
    //testdetectptr.size = 1;

    // = &testdetect;

    asn_long2INTEGER(&message->denm.management.detectionTime, 0);
    asn_long2INTEGER(&denm.management.referenceTime,1);



    auto position = positioning_.position_fix();

    if (!position.confidence) {
        std::cerr << "Skipping DENM, because no good position is available, yet." << std::endl;
        return;
    }
    copydenm(position, denm.management.eventPosition);


    denm.management.stationType = StationType_passengerCar;



    std::string error;
    if (!message.validate(error)) {
        throw std::runtime_error("Invalid high frequency DENM: %s" + error);
    }

    if (print_tx_msg_) {
        std::cout << "Generated DENM contains\n";
        //std::cout<<std::string(&message);
        //const char [] = message;
        //auto test = message;
        print_denm(std::cout, message, "  ", 1);//std::cout, message, "  ", 1
    }
    //std::cout<<(message);
    DownPacketPtr packet { new DownPacket() };
    packet->layer(OsiLayer::Application) = std::move(message);

    DataRequest request;
    request.its_aid = aid::CA;
    request.transport_type = geonet::TransportType::SHB;
    request.communication_profile = geonet::CommunicationProfile::ITS_G5;

    auto confirm = Application::request(request, std::move(packet));
    if (!confirm.accepted()) {
        throw std::runtime_error("DENM application data request failed");
    }
}
