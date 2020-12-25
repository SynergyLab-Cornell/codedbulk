/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author:  Shih-Hao Tseng (st688@cornell.edu)
 */
// The unif random variable that is borrowed from ns-3 
// to ensure the implementation generates the same random values as the 
// simulation

#ifndef UNIF_RANDOM_VARIABLE_H
#define UNIF_RANDOM_VARIABLE_H

#include "simple-ref-count.h"
#include "ptr.h"
#include <stdint.h>

class RngStream
{
public:
  /**
   * Construct from explicit seed, stream and substream values.
   *
   * \param [in] seed The starting seed.
   * \param [in] stream The stream number.
   * \param [in] substream The sub-stream number.
   */
  RngStream (uint32_t seed, uint64_t stream, uint64_t substream);
  /**
   * Copy constructor.
   *
   * \param [in] r The RngStream to copy.
   */
  RngStream (const RngStream & r);
  /**
   * Generate the next random number for this stream.
   * Uniformly distributed between 0 and 1.
   *
   * \returns The next random.
   */
  double RandU01 (void);

private:
  /**
   * Advance \p state of the RNG by leaps and bounds.
   *
   * \param [in] nth The stream or substream index.
   * \param [in] by The log2 base of \p nth.
   * \param [in] state The state vector to advance.
   */
  void AdvanceNthBy (uint64_t nth, int by, double state[6]);

  /** The RNG state vector. */
  double m_currentState[6];
};

class UniformRandomVariable : public SimpleRefCount<UniformRandomVariable>
{
public:
  /**
   * \brief Creates a uniform distribution RNG with the default range.
   */
  UniformRandomVariable (uint64_t stream = 0) {
      m_rng = new RngStream (1,stream,1);
      m_stream = stream;
  }
  ~UniformRandomVariable () {  delete m_rng;  }

  /**
   * \brief Get the lower bound on randoms returned by GetValue(void).
   * \return The lower bound on values from GetValue(void).
   */
  double GetMin (void) const;

  /**
   * \brief Get the upper bound on values returned by GetValue(void).
   * \return The upper bound on values from GetValue(void).
   */
  double GetMax (void) const;

  /**
   * \brief Get the next random value, as a double in the specified range
   * \f$[min, max)\f$.
   *
   * \note The upper limit is excluded from the output range.
   *
   * \param [in] min Low end of the range (included).
   * \param [in] max High end of the range (excluded).
   * \return A floating point random value.
   */
  double GetValue (double min, double max);

  /**
   * \brief Get the next random value, as an unsigned integer in the
   * specified range \f$[min, max]/f$.
   *
   * \note The upper limit is included in the output range.
   *
   * \param [in] min Low end of the range.
   * \param [in] max High end of the range.
   * \return A random unsigned integer value.
   */
  uint32_t GetInteger (uint32_t min, uint32_t max);

  // Inherited from RandomVariableStream
  /**
   * \brief Get the next random value as a double drawn from the distribution.
   * \return A floating point random value.
   * \note The upper limit is excluded from the output range.
  */
  virtual double GetValue (void);
  /**
   * \brief Get the next random value as an integer drawn from the distribution.
   * \return  An integer random value.
   * \note The upper limit is included in the output range.
   */
  virtual uint32_t GetInteger (void);
  
private:
  /** The lower bound on values that can be returned by this RNG stream. */
  double m_min;

  /** The upper bound on values that can be returned by this RNG stream. */
  double m_max;

  RngStream *m_rng;
  int64_t m_stream;
};  // class UniformRandomVariable

#endif /* UNIF_RANDOM_VARIABLE_H */