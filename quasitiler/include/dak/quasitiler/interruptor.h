#pragma once

#ifndef DAK_QUASITILER_INTERRUPTOR_H
#define DAK_QUASITILER_INTERRUPTOR_H

namespace dak::quasitiler
{
   ////////////////////////////////////////////////////////////////////////////
   //
   // Interrupt (stop) something.

   struct interruptor_t
   {
      virtual ~interruptor_t() = default;

      virtual bool interrupted() = 0;
   };
}

#endif /* DAK_QUASITILER_INTERRUPTOR_H */
