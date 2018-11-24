using System;
using System.Collections;
using UnityEngine;

namespace Adrenak.GPUVideoPlayer {
	public class GPUVideoPlayer : MonoBehaviour {
		public enum State {
			Idle,
			Loaded,
			Failed,
			Playing,
			Paused,
			Stopped,
			Ended
		}

		Plugin.StateChangedCallback m_NativeCallback;

		/// <summary>
		/// Returns the <see cref="Description"/> data for the media being played
		/// </summary>
		public Description MediaDescription {
			get { return m_Description; }
		}
		Description m_Description;

		/// <summary>
		/// Returns a reference of the Texture2D object on which the video frames is updated
		/// </summary>
		public Texture2D MediaTexture {
			get { return m_Texture; }
		}
		Texture2D m_Texture;

		/// <summary>
		/// The current state of the video player
		/// </summary>
		public State MediaState {
			get { return m_State; }
		}
		State m_State;

		public StateUnityEvent onStateChanged = new StateUnityEvent();

		[Header("Auto Play Configuration")]
		public bool autoPlay;
		public string autoPath;

		// ================================================
		// EXPOSED API
		// ================================================

		/// <summary>
		/// Loads the video at the given path (or URL)
		/// </summary>
		/// <param name="path"></param>
		public void Load(string path) {
			m_NativeCallback = new Plugin.StateChangedCallback(HandleStateChange);

			if (Plugin.CreateMediaPlayback(m_NativeCallback) != 0)
				LogError("Could not create media playback");

			if (Plugin.LoadContent(path) != 0)
				LogError("Could not load path");
		}
		/// <summary>
		/// Plays (or resumes) the video playback.
		/// </summary>
		/// <returns>Whether the play attempts was successful</returns>
		public bool Play() {
			if (Plugin.Play() != 0) {
				LogError("Cannot play video");
				return false;
			}
			if (m_Texture == null && CreateTexture(m_Description.width, m_Description.height)) {
				ChangeState(State.Playing);
				return true;
			}
			return false;
		}

		/// <summary>
		/// Pauses the video playback
		/// </summary>
		/// <returns>Whether the pause attempt was successful</returns>
		public bool Pause() {
			if (Plugin.Pause() != 0) {
				LogError("Could not pause");
				return false;
			}
			ChangeState(State.Paused);
			return true;
		}

		/// <summary>
		/// Stops the video playback and unloads the video
		/// </summary>
		/// <returns>Whether the stop attempt was successful</returns>
		public bool Stop() {
			if (Plugin.Stop() != 0) {
				LogError("Could not stop the video");
				return false;
			}

			ChangeState(State.Stopped);
			Unload();
			return true;
		}

		/// <summary>
		/// Returns the rate of playback. Eg. 1x
		/// </summary>
		/// <returns>The playback rate. -1 if there was an error</returns>
		public double GetPlaybackRate() {
			double rate;
			if (Plugin.GetPlaybackRate(out rate) != 0) {
				LogError("Could not get playback rate");
				return -1;
			}
			return rate;
		}

		/// <summary>
		/// Gets the duration of the video. In 1/10^7 seconds. So a 60 second video will return 600000000
		/// </summary>
		/// <returns>The duration of the video</returns>
		public long GetDuration() {
			long duration;
			if (Plugin.GetDuration(out duration) != 0) {
				LogError("Could not get duration");
				return -1;
			}
			return duration;
		}

		/// <summary>
		/// Sets the position of the video player to the ratio set. Eg. .5f would set it to halfway
		/// </summary>
		/// <param name="percent"></param>
		/// <returns>Whether the seek attempt was successful</returns>
		public bool SeekByRatio(float percent) {
			if (percent < 0 || percent > 1) {
				LogError("Passed percentage value cannot be higher than 1");
				return false;
			}

			return SeekByTime((long)(percent * GetDuration()));
		}

		/// <summary>
		/// Sets the position of the video player to the time given. Time is in 1/10^7 second units. So 600000000 will set it to 60 seconds since the start of the video
		/// </summary>
		/// <param name="position"></param>
		/// <returns>Whether the seek attempt was successful</returns>
		public bool SeekByTime(long position) {
			if (Plugin.SetPosition(position) != 0) {
				LogError("Could not set position");
				return false;
			}
			return true;
		}

		/// <summary>
		/// Gets the current position of the video player in 1/10^7 seconds. 
		/// </summary>
		/// <returns></returns>
		public long GetPosition() {
			long position;
			if (Plugin.GetPosition(out position) != 0) {
				LogError("Could not get position");
				return -1;
			}
			return position;
		}

		// ================================================
		// INTERNAL METHODS
		// ================================================
		IEnumerator Start() {
			if (autoPlay) Load(autoPath);
			onStateChanged.AddListener(state => {
				if (autoPlay) Play();
			});

			while (true) {
				yield return new WaitForEndOfFrame();
				Plugin.SetTimeFromUnity(Time.timeSinceLevelLoad);
				GL.IssuePluginEvent(Plugin.GetRenderEventFunc(), 1);
			}

		}

		bool CreateTexture(uint width, uint height) {
			var nativeTexture = IntPtr.Zero;
			if (Plugin.CreatePlaybackTexture((uint)width, (uint)height, out nativeTexture) != 0) {
				LogError("Could not create playback texture");
				return false;
			}

			m_Texture = Texture2D.CreateExternalTexture((int)width, (int)height, TextureFormat.RGBA32, false, false, nativeTexture);
			if (m_Texture == null) {
				LogError("Could not create external texture");
				return false;
			}
			return true;
		}

		void Unload() {
			Plugin.ReleaseMediaPlayback();
			m_Texture = null;
		}

		void HandleStateChange(Plugin.StateChangedMessage args) {
			var stateType = (StateType)Enum.ToObject(typeof(StateType), args.type);

			switch (stateType) {
				case StateType.Opened:
					m_Description = args.description;
					ChangeState(State.Loaded);
					break;
				case StateType.Failed:
					ChangeState(State.Failed);
					break;
				case StateType.StateChanged:
					var playbackState = (PlaybackState)Enum.ToObject(typeof(PlaybackState), args.state);
					if (playbackState == PlaybackState.Ended) {
						Unload();
						ChangeState(State.Ended);
					}
					break;
			}
		}

		void OnDisable() {
			Unload();
		}

		void ChangeState(State state) {
			m_State = state;
			onStateChanged.Invoke(m_State);
		}

		void LogError(object error) {
			Debug.LogError("[GPUVideoPlayer] " + error);
		}

		// ================================================
		// INTERNAL ENUMS
		// ================================================
		enum StateType {
			None = 0,
			Opened,
			StateChanged,
			Failed,
		}

		enum PlaybackState {
			None = 0,
			Opening,
			Buffering,
			Playing,
			Paused,
			Ended
		}
	}
}
