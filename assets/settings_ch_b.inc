const size_t START_ITERS_CNT = 64;
const double START_R_CIRCLE_INF = 10;
const double START_SCALE = 550;

Uint32 get_color(size_t iter)
{
  return (iter == START_ITERS_CNT ? 0xFFFFFFFFFF : 0xFF0000000);
}