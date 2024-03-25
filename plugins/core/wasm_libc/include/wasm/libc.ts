// Copyright Chad Engler

import { char, i32, ptr, u32, f64, const_ptr } from 'he/core/ctypes';
import { lib } from 'he/core/lib';
import { Pointer } from 'he/core/pointer';

interface tm {};

class TmPtr
{
    constructor(public ptr: ptr<tm>) {}

    get tm_sec(): i32 { return Pointer.readInt32(this.ptr as ptr<i32>); }
    get tm_min(): i32 { return Pointer.readInt32((this.ptr + 4) as ptr<i32>); }
    get tm_hour(): i32 { return Pointer.readInt32((this.ptr + 8) as ptr<i32>); }
    get tm_mday(): i32 { return Pointer.readInt32((this.ptr + 12) as ptr<i32>); }
    get tm_mon(): i32 { return Pointer.readInt32((this.ptr + 16) as ptr<i32>); }
    get tm_year(): i32 { return Pointer.readInt32((this.ptr + 20) as ptr<i32>); }
    get tm_wday(): i32 { return Pointer.readInt32((this.ptr + 24) as ptr<i32>); }
    get tm_yday(): i32 { return Pointer.readInt32((this.ptr + 28) as ptr<i32>); }
    get tm_isdst(): i32 { return Pointer.readInt32((this.ptr + 32) as ptr<i32>); }
    get tm_gmtoff(): i32 { return Pointer.readInt32((this.ptr + 36) as ptr<i32>); }

    set tm_sec(value: i32) { Pointer.writeInt32(this.ptr as ptr<i32>, value); }
    set tm_min(value: i32) { Pointer.writeInt32((this.ptr + 4) as ptr<i32>, value); }
    set tm_hour(value: i32) { Pointer.writeInt32((this.ptr + 8) as ptr<i32>, value); }
    set tm_mday(value: i32) { Pointer.writeInt32((this.ptr + 12) as ptr<i32>, value); }
    set tm_mon(value: i32) { Pointer.writeInt32((this.ptr + 16) as ptr<i32>, value); }
    set tm_year(value: i32) { Pointer.writeInt32((this.ptr + 20) as ptr<i32>, value); }
    set tm_wday(value: i32) { Pointer.writeInt32((this.ptr + 24) as ptr<i32>, value); }
    set tm_yday(value: i32) { Pointer.writeInt32((this.ptr + 28) as ptr<i32>, value); }
    set tm_isdst(value: i32) { Pointer.writeInt32((this.ptr + 32) as ptr<i32>, value); }
    set tm_gmtoff(value: i32) { Pointer.writeInt32((this.ptr + 36) as ptr<i32>, value); }
}

const MONTH_DAYS_REGULAR_CUMULATIVE = [0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334];
const MONTH_DAYS_LEAP_CUMULATIVE = [0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335];
const TZNAME_MAX = 6;

function isLeapYear(year: number): boolean
{
    return (year % 4) === 0 && ((year % 100) !== 0 || (year % 400) === 0);
}

function ydayFromDate(date: Date): number
{
    const leap = isLeapYear(date.getFullYear());
    const monthDaysCumulative = (leap ? MONTH_DAYS_LEAP_CUMULATIVE : MONTH_DAYS_REGULAR_CUMULATIVE);
    const yday = monthDaysCumulative[date.getMonth()] + date.getDate() - 1; // -1 since it's days since Jan 1
    return yday;
}

lib.addImports('libc', {
    heWASM_TzSet: function (tz: ptr<i32>, dst: ptr<i32>, stdName: ptr<char>, stdNameLen: u32, dstName: ptr<char>, dstNameLen: u32): void
    {
        const currentYear = new Date().getFullYear();
        const winter = new Date(currentYear, 0, 1);
        const summer = new Date(currentYear, 6, 1);
        const winterOffset = winter.getTimezoneOffset();
        const summerOffset = summer.getTimezoneOffset();

        // Local standard timezone offset. Local standard time is not adjusted for daylight
        // savings. This code uses the fact that getTimezoneOffset returns a greater value during
        // Standard Time versus Daylight Saving Time (DST). Thus it determines the expected output
        // during Standard Time, and it compares whether the output of the given date the same
        // (Standard) or less (DST).
        const stdTimezoneOffset = Math.max(winterOffset, summerOffset);

        // timezone is specified as seconds west of UTC ("The external variable
        // `timezone` shall be set to the difference, in seconds, between
        // Coordinated Universal Time (UTC) and local standard time."), the same
        // as returned by stdTimezoneOffset.
        // See http://pubs.opengroup.org/onlinepubs/009695399/functions/tzset.html
        Pointer.writeInt32(tz, (stdTimezoneOffset * 60) as i32);
        Pointer.writeInt32(dst, (winterOffset !== summerOffset ? 1 : 0) as i32);

        function extractZoneName(date: Date): string
        {
            var match = date.toTimeString().match(/\(([A-Za-z ]+)\)$/);
            return match ? match[1] : "GMT";
        };
        const winterName = extractZoneName(winter);
        const summerName = extractZoneName(summer);

        if (summerOffset < winterOffset)
        {
            // Northern hemisphere
            Pointer.writeString(winterName, stdName, stdNameLen);
            Pointer.writeString(summerName, dstName, dstNameLen);
        }
        else
        {
            Pointer.writeString(winterName, dstName, dstNameLen);
            Pointer.writeString(summerName, stdName, stdNameLen);
        }
    },
    heWASM_MkTime: function (t: ptr<tm>): i32
    {
        const value = new TmPtr(t);
        const date = new Date(value.tm_year, value.tm_mon, value.tm_mday, value.tm_hour, value.tm_min, value.tm_sec, 0);

        // There's an ambiguous hour when the time goes back; the tm_isdst field is
        // used to disambiguate it. Date() basically guesses, so we fix it up if it
        // guessed wrong, or fill in tm_isdst with the guess if it's -1.
        const dst = value.tm_isdst;
        const guessedOffset = date.getTimezoneOffset();
        const start = new Date(date.getFullYear(), 0, 1);
        const summerOffset = new Date(date.getFullYear(), 6, 1).getTimezoneOffset();
        const winterOffset = start.getTimezoneOffset();
        const dstOffset = Math.min(winterOffset, summerOffset); // DST is in December in South
        if (dst < 0)
        {
            // note: some regions don't have DST at all.
            const dst = (summerOffset !== winterOffset && dstOffset === guessedOffset);
            value.tm_isdst = (dst ? 1 : 0) as i32;
        }
        else if ((dst > 0) !== (dstOffset === guessedOffset))
        {
            const nonDstOffset = Math.max(winterOffset, summerOffset);
            const trueOffset = dst > 0 ? dstOffset : nonDstOffset;
            // Don't try setMinutes(date.getMinutes() + ...) -- it's messed up.
            date.setTime(date.getTime() + ((trueOffset - guessedOffset) * 60000));
        }

        const yday = ydayFromDate(date)|0;

        value.tm_wday = date.getDay() as i32;
        value.tm_yday = yday as i32;
        // To match expected behavior, update fields from date
        value.tm_sec = date.getSeconds() as i32;
        value.tm_min = date.getMinutes() as i32;
        value.tm_hour = date.getHours() as i32;
        value.tm_mday = date.getDate() as i32;
        value.tm_mon = date.getMonth() as i32;
        value.tm_year = (date.getFullYear() - 1900) as i32;

        const timeMs = date.getTime();
        if (isNaN(timeMs))
        {
            return -1 as i32;
        }

        // Return time in microseconds
        return (timeMs / 1000) as i32;
    },
    heWASM_TimeGm: function (t: ptr<tm>): i32
    {
        const value = new TmPtr(t);
        const time = Date.UTC(value.tm_year, value.tm_mon, value.tm_mday, value.tm_hour, value.tm_min, value.tm_sec, 0);
        const date = new Date(time);

        value.tm_wday = date.getUTCDay() as i32;

        const start = Date.UTC(date.getUTCFullYear(), 0, 1, 0, 0, 0, 0);
        const yday = ((date.getTime() - start) / (1000 * 60 * 60 * 24))|0;
        value.tm_yday = yday as i32;

        return (date.getTime() / 1000) as i32;
    },
    heWASM_GmTime: function (time: i32, t: ptr<tm>): void
    {
        const value = new TmPtr(t);
        const date = new Date(time * 1000);
        value.tm_sec = date.getUTCSeconds() as i32;
        value.tm_min = date.getUTCMinutes() as i32;
        value.tm_hour = date.getUTCHours() as i32;
        value.tm_mday = date.getUTCDate() as i32;
        value.tm_mon = date.getUTCMonth() as i32;
        value.tm_year = (date.getUTCFullYear() - 1900) as i32;
        value.tm_wday = date.getUTCDay() as i32;

        const start = Date.UTC(date.getUTCFullYear(), 0, 1, 0, 0, 0, 0);
        const yday = ((date.getTime() - start) / (1000 * 60 * 60 * 24))|0;
        value.tm_yday = yday as i32;
    },
    heWASM_LocalTime: function (time: i32, t: ptr<tm>): void
    {
        const value = new TmPtr(t);
        const date = new Date(time * 1000);
        value.tm_sec = date.getSeconds() as i32;
        value.tm_min = date.getMinutes() as i32;
        value.tm_hour = date.getHours() as i32;
        value.tm_mday = date.getDate() as i32;
        value.tm_mon = date.getMonth() as i32;
        value.tm_year = (date.getFullYear() - 1900) as i32;
        value.tm_wday = date.getDay() as i32;

        const yday = ydayFromDate(date)|0;
        value.tm_yday = yday as i32;
        value.tm_gmtoff = -(date.getTimezoneOffset() * 60) as i32;

        // note: DST is in December in South, and some regions don't have DST at all.
        const start = new Date(date.getFullYear(), 0, 1);
        const summerOffset = new Date(date.getFullYear(), 6, 1).getTimezoneOffset();
        const winterOffset = start.getTimezoneOffset();
        const dst = (summerOffset !== winterOffset && date.getTimezoneOffset() === Math.min(winterOffset, summerOffset));
        value.tm_isdst = (dst ? 1 : 0) as i32;
    },

    heWASM_Acos: function (x: f64): f64 { return Math.acos(x) as f64; },
    heWASM_Asin: function (x: f64): f64 { return Math.asin(x) as f64; },
    heWASM_Atan: function (x: f64): f64 { return Math.atan(x) as f64; },
    heWASM_Atan2: function (y: f64, x: f64): f64 { return Math.atan2(y, x) as f64; },
    heWASM_Cos: function (x: f64): f64 { return Math.cos(x) as f64; },
    heWASM_Exp: function (x: f64): f64 { return Math.exp(x) as f64; },
    heWASM_Mod: function (x: f64, y: f64): f64 { return (x % y) as f64; },
    heWASM_Log: function (x: f64): f64 { return Math.log(x) as f64; },
    heWASM_Log10: function (x: f64): f64 { return Math.log10(x) as f64; },
    heWASM_Log1p: function (x: f64): f64 { return Math.log1p(x) as f64; },
    heWASM_Log2: function (x: f64): f64 { return Math.log2(x) as f64; },
    heWASM_Pow: function (x: f64, y: f64): f64 { return Math.pow(x, y) as f64; },
    heWASM_Round: function (x: f64): f64 { return Math.round(x) as f64; },
    heWASM_Sin: function (x: f64): f64 { return Math.sin(x) as f64; },
    heWASM_Tan: function (x: f64): f64 { return Math.tan(x) as f64; },

    heWASM_StdIoWrite: function (fd: i32, src: const_ptr<char>, len: u32)
    {
        const str = Pointer.readString(src, len);
        switch (fd)
        {
            case 1: // stdout
                console.log(str);
                break;
            case 2: // stderr
                console.error(str);
                break;
        }
    },
});
