create table employee (
	id integer,
	country_id char(2),
	mgr_id integer,
	salery integer,
	first_name char(20),
	middle char(1),
	last_name char(20),
	primary key (id, country_id)
);

create table country (
   country_id char(2),
   short_name char(20),
   long_name char(50),
   primary key (country_id)
);

create table department (
   id integer,
   primary key(id),
   name char(25),country_id char(2)
);
