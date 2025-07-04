'''A simple sample environment. Use this as a template for your own envs.'''

import gymnasium
import numpy as np

import pufferlib
from pufferlib.ocean.radarxs import binding

class RadarXs(pufferlib.PufferEnv):
    def __init__(self, num_envs=1, render_mode=None, log_interval=128, size=11, buf=None, seed=0):
        self.single_observation_space = gymnasium.spaces.Box(low=0, high=1,
            shape=(size*size,), dtype=np.uint8)
        self.single_action_space = gymnasium.spaces.Discrete(5)
        self.render_mode = render_mode
        self.num_agents = num_envs
        self.log_interval = log_interval

        super().__init__(buf)
        self.c_envs = binding.vec_init(self.observations, self.actions, self.rewards,
            self.terminals, self.truncations, num_envs, seed, size=size)
 
    def reset(self, seed=0):
        binding.vec_reset(self.c_envs, seed)
        self.tick = 0
        return self.observations, []

    def step(self, actions):
        self.tick += 1

        self.actions[:] = actions
        binding.vec_step(self.c_envs)

        info = []
        if self.tick % self.log_interval == 0:
            info.append(binding.vec_log(self.c_envs))

        return (self.observations, self.rewards,
            self.terminals, self.truncations, info)

    def render(self):
        binding.vec_render(self.c_envs, 0)

    def close(self):
        binding.vec_close(self.c_envs)

if __name__ == '__main__':
    N = 4096

    env = RadarXs(num_envs=N)
    env.reset()
    steps = 0

    CACHE = 1024
    actions = np.random.randint(0, 5, (CACHE, N))

    i = 0
    import time
    start = time.time()
    while time.time() - start < 10:
        env.step(actions[i % CACHE])
        steps += N
        i += 1

    print('RadarXs SPS:', int(steps / (time.time() - start)))


##########################################
# Ye Olde radars.py 
##########################################

# """A simple sample environment. Use this as a template for your own envs."""

# import gymnasium
# import numpy as np

# import pufferlib
# from pufferlib.ocean.radars import binding


# # Time is always milliseconds.
# # Distance is always millimeters.
# # Velocities are millimeters per millisecond, equal to meter per second (m/s).

# MAX_AZ_SLICES = 30
# MAX_EL_SLICES = 10

# MAX_SEARCHERS = 1
# FEATURES_PER_TRACKER = 3

# PLACEHOLDER_FOR_SENSOR_ID = 1

# MAX_EARLY = 30000  # 30 seconds
# MAX_TARDY = -30000  # -30 seconds


# class Radars(pufferlib.PufferEnv):
#     def __init__(
#         self,
#         num_envs=1,
#         render_mode=None,
#         buf=None,
#         initial_targets=200,
#         max_trackers=200,
#     ):
#         self.single_observation_space = gymnasium.spaces.Box(
#             low=MAX_TARDY,
#             high=MAX_EARLY,
#             shape=(
#                 MAX_AZ_SLICES * MAX_EL_SLICES
#                 + max_trackers * FEATURES_PER_TRACKER
#                 + PLACEHOLDER_FOR_SENSOR_ID,
#             ),
#             dtype=np.int16,
#         )
#         self.single_action_space = gymnasium.spaces.Discrete(
#             MAX_SEARCHERS + max_trackers
#         )
#         self.render_mode = render_mode
#         self.num_agents = num_envs
#         self.max_trackers = max_trackers

#         super().__init__(buf)
#         self.c_envs = binding.Radars(
#             self.observations,
#             self.actions,
#             self.rewards,
#             self.terminals,
#             num_envs,
#             initial_targets,
#             max_trackers,
#         )

#     def reset(self, seed=None):
#         self.c_envs.reset()
#         return self.observations, []

#     def step(self, actions):
#         self.actions[:] = actions
#         self.c_envs.step()

#         episode_returns = self.rewards[self.terminals]

#         info = []
#         if len(episode_returns) > 0:
#             info = [
#                 {
#                     "reward": np.mean(episode_returns),
#                 }
#             ]

#         return (self.observations, self.rewards, self.terminals, self.truncations, info)

#     def render(self):
#         self.c_envs.render()

#     def close(self):
#         self.c_envs.close()


# def test_performance(timeout=100):
#     env = Radars(max_trackers=300, initial_targets=300)
#     env.reset()
#     tick = 0

#     import time

#     start = time.time()
#     while time.time() - start < timeout:
#         env.step(0)
#         env.render()
#         tick += 1
#         env.step(np.random.randint(0, 1 + env.max_trackers, size=env.num_agents))
#         env.render()
#         tick += 1

#     print(f"SPS: {tick / (time.time() - start)}")


# if __name__ == "__main__":
#     test_performance()
