/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Trinity College Dublin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Rohit P. Tahiliani <rohit.tahil@gmail.com>
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

class PiSquareQueueDiscTestItem : public QueueDiscItem {
public:
  PiSquareQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint16_t protocol);
  virtual ~PiSquareQueueDiscTestItem ();
  virtual void AddHeader (void);

private:
  PiSquareQueueDiscTestItem ();
  PiSquareQueueDiscTestItem (const PiSquareQueueDiscTestItem &);
  PiSquareQueueDiscTestItem &operator = (const PiSquareQueueDiscTestItem &);
};

PiSquareQueueDiscTestItem::PiSquareQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint16_t protocol)
  : QueueDiscItem (p, addr, protocol)
{
}

PiSquareQueueDiscTestItem::~PiSquareQueueDiscTestItem ()
{
}

void
PiSquareQueueDiscTestItem::AddHeader (void)
{
}

class PiSquareQueueDiscTestCase : public TestCase
{
public:
  PiSquareQueueDiscTestCase ();
  virtual void DoRun (void);
private:
  void Enqueue (Ptr<PiSquareQueueDisc> queue, uint32_t size, uint32_t nPkt);
  void EnqueueWithDelay (Ptr<PiSquareQueueDisc> queue, uint32_t size, uint32_t nPkt);
  void RunPiSquareTest (StringValue mode);
};

PiSquareQueueDiscTestCase::PiSquareQueueDiscTestCase ()
  : TestCase ("Sanity check on the PI Square queue disc implementation")
{
}

void
PiSquareQueueDiscTestCase::RunPiSquareTest (StringValue mode)
{
  uint32_t pktSize = 0;
  // 1 for packets; pktSize for bytes
  uint32_t modeSize = 1;
  uint32_t qSize = 8;
  Ptr<PiSquareQueueDisc> queue = CreateObject<PiSquareQueueDisc> ();

  // test 1: simple enqueue/dequeue with no drops
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", DoubleValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("A", DoubleValue (0.125)), true,
                         "Verify that we can actually set the attribute A");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("B", DoubleValue (1.25)), true,
                         "Verify that we can actually set the attribute B");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MeanPktSize", UintegerValue (1000)), true,
                         "Verify that we can actually set the attribute MeanPktSize");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueRef", DoubleValue (4)), true,
                         "Verify that we can actually set the attribute QueueRef");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("W", DoubleValue (125)), true,
                         "Verify that we can actually set the attribute W");

  Address dest;

  if (queue->GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      pktSize = 500;
      modeSize = pktSize;
      queue->SetQueueLimit (qSize * modeSize);
    }

  Ptr<Packet> p1, p2, p3, p4, p5, p6, p7, p8;
  p1 = Create<Packet> (pktSize);
  p2 = Create<Packet> (pktSize);
  p3 = Create<Packet> (pktSize);
  p4 = Create<Packet> (pktSize);
  p5 = Create<Packet> (pktSize);
  p6 = Create<Packet> (pktSize);
  p7 = Create<Packet> (pktSize);
  p8 = Create<Packet> (pktSize);

  queue->Initialize ();
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 0 * modeSize, "There should be no packets in there");
  queue->Enqueue (Create<PiSquareQueueDiscTestItem> (p1, dest, 0));
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 1 * modeSize, "There should be one packet in there");
  queue->Enqueue (Create<PiSquareQueueDiscTestItem> (p2, dest, 0));
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 2 * modeSize, "There should be two packets in there");
  queue->Enqueue (Create<PiSquareQueueDiscTestItem> (p3, dest, 0));
  queue->Enqueue (Create<PiSquareQueueDiscTestItem> (p4, dest, 0));
  queue->Enqueue (Create<PiSquareQueueDiscTestItem> (p5, dest, 0));
  queue->Enqueue (Create<PiSquareQueueDiscTestItem> (p6, dest, 0));
  queue->Enqueue (Create<PiSquareQueueDiscTestItem> (p7, dest, 0));
  queue->Enqueue (Create<PiSquareQueueDiscTestItem> (p8, dest, 0));
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 8 * modeSize, "There should be eight packets in there");

  Ptr<QueueDiscItem> item;

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the first packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 7 * modeSize, "There should be seven packets in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p1->GetUid (), "was this the first packet ?");

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the second packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 6 * modeSize, "There should be six packet in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p2->GetUid (), "Was this the second packet ?");

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the third packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 5 * modeSize, "There should be five packets in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p3->GetUid (), "Was this the third packet ?");

  item = queue->Dequeue ();
  item = queue->Dequeue ();
  item = queue->Dequeue ();
  item = queue->Dequeue ();
  item = queue->Dequeue ();

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item == 0), true, "There are really no packets in there");

  struct d {
    uint32_t test2;
    uint32_t test3;
  } drop;


  // test 2: default values for PI parameters
  queue = CreateObject<PiSquareQueueDisc> ();
  qSize = 300 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MeanPktSize", UintegerValue (pktSize)), true,
                         "Verify that we can actually set the attribute MeanPktSize");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueRef", DoubleValue (50)), true,
                         "Verify that we can actually set the attribute QueueRef");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", DoubleValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("W", DoubleValue (170)), true,
                         "Verify that we can actually set the attribute W");
  queue->Initialize ();
  EnqueueWithDelay (queue, pktSize, 300);
  Simulator::Stop (Seconds (40));
  Simulator::Run ();
  PiSquareQueueDisc::Stats st = StaticCast<PiSquareQueueDisc> (queue)->GetStats ();
  drop.test2 = st.unforcedDrop;
  NS_TEST_EXPECT_MSG_NE (drop.test2, 0, "There should be some unforced drops");


  // test 3: high value of W
  queue = CreateObject<PiSquareQueueDisc> ();
  qSize = 300 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MeanPktSize", UintegerValue (pktSize)), true,
                         "Verify that we can actually set the attribute MeanPktSize");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueRef", DoubleValue (50)), true,
                         "Verify that we can actually set the attribute QueueRef");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", DoubleValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("W", DoubleValue (500)), true,
                         "Verify that we can actually set the attribute W");
  queue->Initialize ();
  EnqueueWithDelay (queue, pktSize, 300);
  Simulator::Stop (Seconds (40));
  Simulator::Run ();
  st = StaticCast<PiSquareQueueDisc> (queue)->GetStats ();
  drop.test3 = st.unforcedDrop;
  NS_TEST_EXPECT_MSG_GT (drop.test3, drop.test2, "Test 3 should have more unforced drops than Test 2");
}

void 
PiSquareQueueDiscTestCase::Enqueue (Ptr<PiSquareQueueDisc> queue, uint32_t size, uint32_t nPkt)
{
  Address dest;
  for (uint32_t i = 0; i < nPkt; i++)
    {
      queue->Enqueue (Create<PiSquareQueueDiscTestItem> (Create<Packet> (size), dest, 0));
    }
}

void 
PiSquareQueueDiscTestCase::EnqueueWithDelay (Ptr<PiSquareQueueDisc> queue, uint32_t size, uint32_t nPkt)
{
  Address dest;
  double delay = 0.1;
  for (uint32_t i=0;i<nPkt;i++)
  {
      Simulator::Schedule (Time (Seconds ((i + 1) * delay)), &PiSquareQueueDiscTestCase::Enqueue,this,queue,size, 1);
  }

}

void
PiSquareQueueDiscTestCase::DoRun (void)
{
  RunPiSquareTest (StringValue ("QUEUE_MODE_PACKETS"));
  RunPiSquareTest (StringValue ("QUEUE_MODE_BYTES"));
  Simulator::Destroy ();
}

static class PiSquareQueueDiscTestSuite : public TestSuite
{
public:
  PiSquareQueueDiscTestSuite ()
    : TestSuite ("pi-square-queue-disc", UNIT)
  {
    AddTestCase (new PiSquareQueueDiscTestCase (), TestCase::QUICK);
  }
} g_piSquareQueueTestSuite;
