class TimeJs {
  #reset() {
    this.__memory = undefined;
  }

  constructor() {
    this.#reset();
  }

  init(wasm) {
    this.__memory = wasm.instance.exports.memory;
  }

  clock = () => BigInt(Date.now());
  time = (timer_ptr) => {
    if (timer_ptr != 0) {
      var buf = new BigInt64Array(this.__memory.buffer, timer_ptr, 1);
      buf[0] = BigInt(Date.now());
    }

    return BigInt(Date.now());
  };

  create_date(time_num) {
    let num = Number(time_num);
    return new Date(num);
  }

  // Get UTC time
  _get_year = (time_num) => {
    // console.log(time_num)
    return this.create_date(time_num).getUTCFullYear();
  };
  _get_month = (time_num) => {
    return this.create_date(time_num).getUTCMonth();
  };
  _get_day = (time_num) => {
    return this.create_date(time_num).getUTCDay();
  };
  _get_day_of_month = (time_num) => {
    return this.create_date(time_num).getUTCDate();
  };
  _get_days_since_year = (time_num) => {
    const date = this.create_date(time_num);
    return (
      (Date.UTC(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate()) -
        Date.UTC(date.getUTCFullYear(), 0, 0)) /
      24 /
      60 /
      60 /
      1000
    );
  };
  _get_hours = (time_num) => {
    return this.create_date(time_num).getUTCHours();
  };
  _get_minutes = (time_num) => {
    return this.create_date(time_num).getUTCMinutes();
  };
  _get_seconds = (time_num) => {
    return this.create_date(time_num).getUTCSeconds();
  };

  // Get Local time
  _get_local_year = (time_num) => {
    return this.create_date(time_num).getFullYear();
  };
  _get_local_month = (time_num) => {
    return this.create_date(time_num).getMonth();
  };
  _get_local_day = (time_num) => {
    return this.create_date(time_num).getDay();
  };
  _get_local_day_of_month = (time_num) => {
    return this.create_date(time_num).getDate();
  };
  _get_local_days_since_year = (time_num) => {
    const date = this.create_date(time_num);
    return (
      (Date.UTC(date.getFullYear(), date.getMonth(), date.getDate()) -
        Date.UTC(date.getFullYear(), 0, 0)) /
      24 /
      60 /
      60 /
      1000
    );
  };
  _get_local_hours = (time_num) => {
    return this.create_date(time_num).getHours();
  };
  _get_local_minutes = (time_num) => {
    return this.create_date(time_num).getMinutes();
  };
  _get_local_seconds = (time_num) => {
    return this.create_date(time_num).getSeconds();
  };

  _get_weeks_in_year = (year) => {
    let last_date_of_year = new Date(year, 11, 31);
    let d = new Date(+last_date_of_year);
    d.setHours(0, 0, 0, 0);
    d.setDate(d.getDate() + 4 - (d.getDay() || 7));
    let yearStart = new Date(d.getFullYear(), 0, 1);
    let weekNo = Math.ceil(((d - yearStart) / 86400000 + 1) / 7);
    return weekNo == 1 ? 52 : 53;
  };
  _get_timezone_offset = (time_num) => {
    return this.create_date(time_num).getTimezoneOffset();
  };
}
