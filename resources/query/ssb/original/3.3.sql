select c_city,
       s_city,
       d_year,
       sum(lo_revenue) as revenue
from supplier,
     lineorder,
     "date",
     customer
where lo_custkey = c_custkey
  and lo_suppkey = s_suppkey
  and lo_orderdate = d_datekey
  and (c_city = 'UNITED KI1' or c_city = 'UNITED KI5')
  and (s_city = 'UNITED ST0' or s_city = 'UNITED ST9')
  and d_year >= 1992
  and d_year <= 1997
group by c_city, s_city, d_year
order by d_year asc, revenue desc