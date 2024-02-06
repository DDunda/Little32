#pragma once

#ifndef L32_Computer_h_
#define L32_Computer_h_

#include "L32_Types.h"

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <vector>

namespace Little32 
{
	struct ICore;
	struct IDevice;
	struct IMemoryMapped;
	struct IMappedDevice;

	struct Computer
	{
		typedef std::function<void(Computer&)> IntervalFunction;

		struct Interval
		{
			size_t cycle_length;
			IntervalFunction callback;
			size_t repeats = 0;
		};

		ICore* core = nullptr;
		std::vector<IDevice*> devices = {};
		std::vector<IMemoryMapped*> mappings = {};
		std::vector<IMappedDevice*> mapped_devices = {};
		std::map<size_t, std::list<std::shared_ptr<Interval>>> intervals = {};
		std::list<std::shared_ptr<Interval>> constant_intervals = {};
		size_t cur_cycle = 0;

		const std::shared_ptr<Interval> AddInterval(const size_t length, const IntervalFunction& interval, size_t repeats = 0)
		{
			// It runs every clock, so we dont want to move this around
			if (length == 1)
			{
				constant_intervals.push_back(std::shared_ptr<Interval>(new Interval{ 1, interval, repeats }));
				return constant_intervals.back();
			}
			
			// Length == 0 is treated like once per overflow;
			// i.e., once it returns to this cur_cycle
			else if (intervals.empty() || intervals.count(cur_cycle + length) == 0)
			{
				auto& i = (intervals[cur_cycle + length] = { std::shared_ptr<Interval>(new Interval{ length, interval, repeats }) });
				return i.front();
			}
			else
			{
				auto& i = intervals[cur_cycle + length];
				i.push_back(std::shared_ptr<Interval>(new Interval { length, interval, repeats }));
				return i.back();
			}
		}

		bool RemoveInterval(const std::shared_ptr<Interval> interval)
		{
			auto it = constant_intervals.begin();
			auto it_end = constant_intervals.end();

			while (it != it_end)
			{
				if ((it++)->get() != interval.get()) continue;

				constant_intervals.erase(it);
				return true;
			}

			for (auto& [num, group] : intervals)
			{
				it = group.begin();
				it_end = group.end();

				do
				{
					if (it->get() != interval.get()) continue;

					intervals[num].erase(it);
					// Don't leave any empty lists behind
					if (intervals[num].empty()) intervals.erase(num);
					return true;
				} while ((++it) != it_end);
			}

			return false;
		}

		inline void CheckIntervals()
		{
			for (auto it = constant_intervals.begin(); it != constant_intervals.end();)
			{
				Interval* const i = it->get();
				i->callback(*this);

				if (i->repeats == 1)
				{
					it = constant_intervals.erase(it);
					continue;
				}

				--(i->repeats);
				++it;
			}

			if (intervals.count(cur_cycle) > 0)
			{
				auto& list = intervals[cur_cycle];

				std::list<std::shared_ptr<Interval>> old_intervals = {};
				old_intervals.swap(list);

				do
				{
					Interval* const i = old_intervals.front().get();

					i->callback(*this);

					if (i->repeats == 1)
					{
						old_intervals.pop_front();
						continue;
					}

					--(i->repeats);

					if (intervals.count(cur_cycle + i->cycle_length) == 0)
					{
						intervals[cur_cycle + i->cycle_length] = {};
					}

					auto& i2 = intervals[cur_cycle + i->cycle_length];

					i2.push_back(old_intervals.front());
					old_intervals.pop_front();
				} while (!old_intervals.empty());

				if (list.empty()) intervals.erase(cur_cycle);
			}
		}

		word start_PC = 0;
		word start_SP = 0;

		Computer() : devices(), mappings(), mapped_devices() {}
		~Computer()
		{
			for (auto& d : devices)
			{
				delete d;
			}
			for (auto& md : mapped_devices)
			{
				delete md;
			}
			for (auto& m : mappings)
			{
				delete m;
			}
			devices.clear();
			mapped_devices.clear();
			mappings.clear();
		}

		/// <summary> Clocks the computer a number of times </summary>
		/// <param name="clocks">Number of times to clock the computer</param>
		void Clock(unsigned clocks);

		/// <summary> Clocks the computer once </summary>
		void Clock();

		word Read(word addr);

		byte ReadByte(word addr);

		void Write(word addr, word value);

		void WriteByte(word addr, byte value);

		// Use to write to anything, even ROM. For programming purposes.
		void WriteForced(word addr, word value);
		void WriteByteForced(word addr, byte value);

		/// <summary> Puts the core back to where it started executing without resetting the whole computer </summary>
		void SoftReset();

		/// <summary> Resets the computer as if it were power cycled </summary>
		void HardReset();

		void AddDevice(IDevice& dev);

		void AddMapping(IMemoryMapped& map);

		void AddMappedDevice(IMappedDevice& dev);
	};
}

#endif