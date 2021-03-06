//
//  Positioner.cpp
//  atlas loc
//
//  Created by Janis on 18.05.15.
//  Copyright (c) 2015 Janis. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include <iostream>
#include <iomanip>

#include "atlas_loc/atlas_types.h"
#include "atlas_loc/Positioner.h"

#include "ros/ros.h"

#include <atlas_msgs/Sample.h>
#include <atlas_msgs/Measurement.h>

#include <tf/transform_datatypes.h>
// PRETTY MATRIX PRINT
std::string sep = "\n----------------------------------------\n";
Eigen::IOFormat HeavyFmt(Eigen::FullPrecision, 0, ", ", ";\n", "[", "]", "[", "]");
//std::cout << sep << "ta1:" << sep << ta1.format(HeavyFmt) << sep;

PositionerTDOA::PositionerTDOA ()
{

}

void PositionerTDOA::initialize(ros::NodeHandle n)
{
    ROS_INFO("Initializing Positioner");

    XmlRpc::XmlRpcValue anchors;
    n.getParam("/atlas/anchor", anchors);
    ROS_ASSERT(anchors.getType() == XmlRpc::XmlRpcValue::TypeStruct);
    for (XmlRpc::XmlRpcValue::ValueStruct::const_iterator it = anchors.begin(); it != anchors.end(); ++it)
    {
        ROS_INFO("Found anchor %s", (it->first).c_str());
        uint64_t eui = std::stoul(it->first, nullptr, 16);

        std::vector<double> p;
        n.getParam("/atlas/anchor/" + it->first + "/pos", p);
        Eigen::Vector3d pos(p.at(0), p.at(1), p.at(2));
        m_anchorPositions.insert(std::pair<uint64_t, Eigen::Vector3d>(eui, pos));
    }

    // EKF Configuration
    if (n.getParam("ekf/imu", m_imu))
    {
        ROS_INFO("PARAM ekf/imu: %d", m_imu);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/imu'");
    }
    if (n.getParam("ekf/constrained", m_constrained))
    {
        ROS_INFO("PARAM ekf/constrained: %d", m_constrained);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/imu'");
    }
    if (n.getParam("ekf/sqd", m_sqd))
    {
        ROS_INFO("PARAM ekf/sqd: %d", m_sqd);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/sqd'");
    }
    if (n.getParam("ekf/dimensions", m_dimensions))
    {
        ROS_INFO("PARAM ekf/dimensions: %d", m_dimensions);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/dimensions'");
    }

    if (n.getParam("ekf/maxZ", m_maxZ))
    {
        ROS_INFO("PARAM ekf/maxZ: %f", m_maxZ);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/maxZ'");
    }

    if (n.getParam("ekf/minZ", m_minZ))
    {
        ROS_INFO("PARAM ekf/minZ: %f", m_minZ);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/minZ'");
    }

    if (n.getParam("ekf/maxY", m_maxY))
    {
        ROS_INFO("PARAM ekf/maxY: %f", m_maxY);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/maxY'");
    }

    if (n.getParam("ekf/minY", m_minY))
    {
        ROS_INFO("PARAM ekf/minY: %f", m_minY);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/minY'");
    }

    if (n.getParam("ekf/maxX", m_maxX))
    {
        ROS_INFO("PARAM ekf/maxX: %f", m_maxX);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/maxX'");
    }

    if (n.getParam("ekf/minX", m_minX))
    {
        ROS_INFO("PARAM ekf/minX: %f", m_minX);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/minX'");
    }

    if (n.getParam("ekf/processNoise", m_processNoise))
    {
        ROS_INFO("PARAM ekf/processNoise: %f", m_processNoise);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/processNoise'");
    }

    if (n.getParam("ekf/measurementVariance", m_measurementNoise))
    {
        ROS_INFO("PARAM ekf/measurementVariance: %f", m_measurementNoise);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/measurementVariance'");
    }

    if (n.getParam("ekf/initialInterval", m_initialInterval))
    {
        ROS_INFO("PARAM ekf/initialInterval: %f", m_initialInterval);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/initialInterval'");
    }

    if (n.getParam("ekf/initialVariance", m_initialStateVariance))
    {
        ROS_INFO("PARAM ekf/initialVariance: %f", m_initialStateVariance);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/initialVariance'");
    }

    if (n.getParam("ekf/initialVarianceDelta", m_initialStateVarianceDelta))
    {
        ROS_INFO("PARAM ekf/initialVarianceDelta: %f", m_initialStateVarianceDelta);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/initialVarianceDelta'");
    }

    if (n.hasParam("ekf/initialPosition"))
    {
        std::vector<double> pos;
        n.getParam("ekf/initialPosition", pos);
        m_initialState.resize(6);
        m_initialState << pos.at(0), pos.at(1), pos.at(2), 0.0, 0.0, 0.0;
        ROS_INFO("PARAM ekf/initialPosition: %f, %f, %f", pos.at(0), pos.at(1), pos.at(2));
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/initialPosition'");
    }

    if (n.getParam("ekf/outlierThreshold", m_outlierThreshold))
    {
        ROS_INFO("PARAM ekf/outlierThreshold: %f", m_outlierThreshold);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/outlierThreshold'");
    }

    if (n.getParam("ekf/outlierThresholdDelta", m_outlierThresholdDelta))
    {
        ROS_INFO("PARAM ekf/outlierThresholdDelta: %f", m_outlierThresholdDelta);
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'ekf/outlierThresholdDelta'");
    }

    if (n.hasParam("orientationOffset"))
    {
        std::vector<double> orientationOffset;
        n.getParam("orientationOffset", orientationOffset);
        Eigen::Vector3d angle(orientationOffset.at(0), orientationOffset.at(1), orientationOffset.at(2));
        m_orientOffset = angle;
        ROS_INFO("PARAM orientationOffset: %f, %f, %f", angle(0), angle(1), angle(2));
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'orientationOffset'");
    }

    // Whitelist Configuration
    if (n.hasParam("whitelist"))
    {
        std::vector<std::string> whitelist;
        n.getParam("whitelist", whitelist);

        for (auto it = whitelist.begin(); it != whitelist.end(); ++it)
        {
            uint64_t tagEUI = std::stoul(*it, nullptr, 16);
            m_subSample.insert(std::make_pair(tagEUI, n.subscribe("/atlas/" + *it + "/sample", 100, &PositionerTDOA::sampleCallback, this)));
            ROS_INFO("PARAM add tag on whitelist: %#lx", (uint64_t)tagEUI);
        }
    }
    else
    {
        ROS_ERROR("PARAM FAILED to get 'whitelist'");
    }
}

void PositionerTDOA::sampleCallback(const atlas_msgs::Sample& msg)
{
    sample_t s;
    s.hts = msg.hts;
    s.txeui = msg.txId;
    s.seq = msg.seq;

    for (auto it = msg.meas.begin(); it != msg.meas.end(); ++it)
    {
        measurement_t m;
        m.ts = it->ts;
        m.toa = it->toa;
        m.lde.maxNoise = it->lde.maxNoise;
        m.lde.firstPathAmp1 = it->lde.firstPathAmp1;
        m.lde.stdNoise = it->lde.stdNoise;
        m.lde.firstPathAmp2 = it->lde.firstPathAmp2;
        m.lde.firstPathAmp3 = it->lde.firstPathAmp3;
        m.lde.maxGrowthCIR = it->lde.maxGrowthCIR;
        m.lde.rxPreamCount = it->lde.rxPreamCount;
        m.lde.firstPath = it->lde.firstPath;

        s.meas.insert(std::make_pair(it->rxId, m));
    }

    m_samples.push_back(s);
}

void PositionerTDOA::extractSamples(std::vector<sample_t> *samples)
{
    while(!m_samples.empty())
    {
        samples->push_back(m_samples.front());
        m_samples.pop_front();
    }
}

void PositionerTDOA::createNewEKF(uint64_t eui, ros::Time ts)
{
    ekf_t ekf;
    ekf.lastUpdate = ts;
    ekf.state = m_initialState;
    ekf.imu = m_imu;
    ekf.sqd = m_sqd;
    ekf.constrained = m_constrained;

    Eigen::MatrixXd stateCovariance = Eigen::MatrixXd::Zero(6, 6);
    stateCovariance.topLeftCorner(3, 3) = Eigen::MatrixXd::Identity(3, 3) * m_initialStateVariance;
    stateCovariance.bottomRightCorner(3, 3) = Eigen::MatrixXd::Identity(3, 3) * m_initialStateVarianceDelta;
    if (m_dimensions == 2)
    {
        stateCovariance(2, 2) = 0;
        stateCovariance(5, 5) = 0;
    }
    ekf.stateCovariance = stateCovariance;

    ekf.W = Eigen::MatrixXd::Identity(3, 3);
    ekf.V = Eigen::MatrixXd::Identity(3, 3);

    m_ekf[eui] = ekf;
}

bool PositionerTDOA::calculatePositionEKFInner(const sample_t &s, position_t *p)
{   
    int count = s.meas.size();
    Eigen::MatrixXd anchorPositions(count, 3);
    Eigen::VectorXd anchorTOAs(count);

    // extracting the tdoa
    int row = 0;
    for (auto it = s.meas.begin(); it != s.meas.end(); ++it)
    {
        anchorPositions.row(row) = m_anchorPositions[it->first].transpose();
        anchorTOAs(row) = it->second.toa;
        row++;
    }

    // --- get the last interval ---
    double interval = m_initialInterval;
    if(m_lastPosition.find(p->eui) != m_lastPosition.end())
    {
        // assuming the first anchor stays the same !!
        double i = (s.hts - m_lastSample[s.txeui].hts).toSec();

        if(i > 0.0)
        {
            interval = i;
        }
        else
        {
            ROS_WARN(" Interval: %.6f smaller or equals zero !!!", i);
        }
    }

    // --- threshold to reinitialize ekf ---
    if(interval > 60.0)
    {
        createNewEKF(s.txeui, s.hts);
        ROS_WARN(" Interval: %.6f greater threshold, reinit EKF !!!", interval);
    }

    // --- get ekf state for specific participant ---
    ekf_t ekf = m_ekf[s.txeui];
    Eigen::VectorXd xk = ekf.state;
    Eigen::MatrixXd Pk = ekf.stateCovariance;
    // --- create dynamic state transition matrix ---
    Eigen::MatrixXd Fk = Eigen::MatrixXd::Identity(6, 6);
    Fk.topRightCorner(3, 3) = Eigen::MatrixXd::Identity(3, 3) * interval;
    if (m_dimensions == 2)
    {
        Fk(2, 5) = 0;
        Fk(5, 5) = 0;
    }

    // --- create dynamic process noise covariance matrix ---
    Eigen::MatrixXd Q = Eigen::MatrixXd::Identity(3,3)*m_processNoise;
    Eigen::MatrixXd Ak(6,3);
    Ak << pow(interval,2)/2, 0, 0,
          0, pow(interval,2)/2, 0,
          0, 0, pow(interval,2)/2,
          interval, 0, 0,
          0, interval, 0,
          0, 0, interval;
    Q = Ak * Q * Ak.transpose();

    if (m_dimensions == 2)
    {
        Q(2, 2) = 0;
        Q(5, 5) = 0;
    }

    // --- prediction ---
    // estimate state
    Eigen::VectorXd exk = Fk * xk;

    // estimate covariance
    Eigen::MatrixXd ePk = Fk * Pk * Fk.transpose() + Q;

    // --- correction ---
    Eigen::Vector3d referenceAnchor = anchorPositions.row(0);

    // observation vector
    Eigen::VectorXd td = anchorTOAs;
    Eigen::VectorXd dv = td.tail(count - 1);

    Eigen::Vector3d ep = exk.head(3);

    Eigen::MatrixXd epmat = ep.transpose().replicate(count - 1, 1);

    Eigen::MatrixXd refmat = referenceAnchor.transpose().replicate(count - 1, 1);

    Eigen::MatrixXd anmat = anchorPositions.bottomRows(count - 1);

    Eigen::MatrixXd ta1 = epmat - anmat;
    Eigen::MatrixXd ta2 = epmat - refmat;

    Eigen::MatrixXd distanceToAnchors = ta1.rowwise().norm();
    Eigen::MatrixXd distanceToReference = ta2.rowwise().norm();
    Eigen::MatrixXd ta1dr = distanceToAnchors.replicate(1, 3);
    Eigen::MatrixXd ta2dr = distanceToReference.replicate(1, 3);
    Eigen::MatrixXd t1 = ta1.cwiseQuotient(ta1dr);
    Eigen::MatrixXd t2 = ta2.cwiseQuotient(ta2dr);
    Eigen::MatrixXd j = t1 - t2;
    Eigen::MatrixXd Hk(j.rows(), j.cols() + 3);
    Eigen::MatrixXd pad = Eigen::MatrixXd::Zero(j.rows(), 3);
    Hk << j, pad;

    // innovation
    Eigen::VectorXd expectedMeasurement = distanceToAnchors - distanceToReference;
    Eigen::VectorXd observedMeasurement = dv;
    Eigen::VectorXd innovation = observedMeasurement - expectedMeasurement;

    // --- create dynamic TDOA measurement noise covariance matrix. ---
    Eigen::MatrixXd Rk;
    //Rk = Eigen::MatrixXd::Identity(count - 1, count - 1) * m_measurementNoise; // original Rk
    Rk.setOnes(count - 1, count - 1);
    Rk += Eigen::MatrixXd::Identity(count - 1, count - 1);

    // covariance of the innovation
    Eigen::MatrixXd Sk = Hk * ePk * Hk.transpose() + Rk;

    // kalman gain computation
    Eigen::MatrixXd Kk = ePk * Hk.transpose() * Sk.inverse();

    // --- state update ---
    // a posteriori state estimate
    xk = exk + Kk * innovation;

    Eigen::ArrayXd xkarray;
    xkarray = xk.array();

    // a posteriori state covariance
    Pk = (Eigen::MatrixXd::Identity(6, 6) - Kk * Hk) * ePk;

    ekf.state = xk;
    ekf.stateCovariance = Pk;

    // constrain min max
    if(ekf.state[0]<m_minX) ekf.state[0]=m_minX;
    if(ekf.state[1]<m_minY) ekf.state[1]=m_minY;
    if(ekf.state[2]<m_minZ) ekf.state[2]=m_minZ;
    if(ekf.state[0]>m_maxX) ekf.state[0]=m_maxX;
    if(ekf.state[1]>m_maxY) ekf.state[1]=m_maxY;
    if(ekf.state[2]>m_maxZ) ekf.state[2]=m_maxZ;

    m_ekf[s.txeui] = ekf;

    p->pos = xk.head(3);
    p->dpos = xk.tail(3);

    return true;
}

bool PositionerTDOA::calculatePositionEKF(const sample_t &s, position_t *p)
{
    if(m_ekf.find(s.txeui) == m_ekf.end())
    {
        createNewEKF(s.txeui, s.hts);
        m_updateCount.insert(std::make_pair(s.txeui, 0));
        m_lastSample.insert(std::make_pair(s.txeui, s));
        return false;
    }

    // reinitialize when diverging
    double threshold = 300;
    if(m_ekf[s.txeui].state.head(3).norm() > threshold)
    {
        ROS_WARN(" Norm: %.2f greater threshold, reinit EKF !!!", threshold);
        createNewEKF(s.txeui, s.hts);
        return false;
    }

    // avoid uninitialized state
    m_updateCount[s.txeui]++;
    if(m_updateCount[s.txeui] < 2)
    {
        return false;
    }

    // set basic position result props
    p->hts = s.hts;
    p->eui = s.txeui;

    p->imu.accel = s.meas.begin()->second.imu.accel;
    p->imu.quat = s.meas.begin()->second.imu.quat;

    bool discard = false;
    if(s.meas.size() < m_minAnchor)
    {
        discard = true;
        return false;
    }

    for (auto it = s.meas.begin(); it != s.meas.end(); ++it)
    {
        if(m_lastSample[s.txeui].meas.find(it->first) != m_lastSample[s.txeui].meas.end())
        {
            //outlier detection
            double lastTdoa = m_lastSample[s.txeui].meas[it->first].toa;
            double tdoa = it->second.toa;
            double diff = fabs(lastTdoa - tdoa);

            if(diff > m_outlierThresholdDelta)
            {
                ROS_WARN(" Outlier: TDOA diff %#lx,%#lx, exceeded d: %.2f", s.txeui, it->first, diff);
                discard = true;
            }

            if(tdoa > m_outlierThreshold || tdoa < -m_outlierThreshold)
            {
                ROS_WARN(" Outlier: TDOA large %#lx,%#lx, exceeded threshold: %.2f", s.txeui, it->first, tdoa);
                discard = true;
            }
        }
    }

    if(!discard)
    {
        if(!calculatePositionEKFInner(s, p))
        {
            discard = true;
        }
    }

    m_lastSample[s.txeui] = s;

    if(discard)
    {
        return false;
    }

    m_lastPosition[p->eui] = *p;

    return true;
}