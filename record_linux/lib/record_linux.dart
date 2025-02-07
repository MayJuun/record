import 'dart:async';
import 'package:flutter/services.dart';
import 'package:record_platform_interface/record_platform_interface.dart';

/// The Linux implementation of the Record plugin.
///
/// This version uses a MethodChannel to communicate with the native
/// Linux plugin (which uses PulseAudio to record audio).
class RecordLinux extends RecordPlatform {
  static void registerWith() {
    RecordPlatform.instance = RecordLinux();
  }

  /// The channel name should match the one used on the native side.
  final MethodChannel _channel = const MethodChannel('record_linux');

  RecordState _state = RecordState.stop;
  StreamController<RecordState>? _stateStreamCtrl;

  @override
  Future<void> create(String recorderId) async {
    // No initialization needed; the native plugin takes care of setup.
  }

  @override
  Future<void> dispose(String recorderId) async {
    _stateStreamCtrl?.close();
    await stop(recorderId);
  }

  @override
  Future<Amplitude> getAmplitude(String recorderId) async {
    // Amplitude reporting is not implemented on Linux.
    return Amplitude(current: -160.0, max: -160.0);
  }

  @override
  Future<bool> hasPermission(String recorderId) async {
    // For Linux, we assume recording permission is always granted.
    return true;
  }

  @override
  Future<bool> isEncoderSupported(String recorderId, AudioEncoder encoder) async {
    // Currently, our native plugin supports only a basic configuration.
    // For example, here we assume only WAV is supported.
    return encoder == AudioEncoder.wav;
  }

  @override
  Future<bool> isPaused(String recorderId) async {
    return _state == RecordState.pause;
  }

  @override
  Future<bool> isRecording(String recorderId) async {
    return _state == RecordState.record;
  }

  @override
  Future<void> pause(String recorderId) async {
    // Pause is not supported in this implementation.
    throw UnsupportedError('Pause is not supported on Linux.');
  }

  @override
  Future<void> resume(String recorderId) async {
    // Resume is not supported in this implementation.
    throw UnsupportedError('Resume is not supported on Linux.');
  }

  @override
  Future<void> start(
    String recorderId,
    RecordConfig config, {
    required String path,
  }) async {
    // The native plugin currently does not write to a file.
    // If needed, you could extend the native code to write to [path].
    try {
      await _channel.invokeMethod('startRecording');
      _updateState(RecordState.record);
    } on PlatformException catch (e) {
      throw Exception('Failed to start recording: ${e.message}');
    }
  }

  @override
  Future<String?> stop(String recorderId) async {
    try {
      await _channel.invokeMethod('stopRecording');
      _updateState(RecordState.stop);
      // Since file saving is not implemented on Linux at this time,
      // we return null.
      return null;
    } on PlatformException catch (e) {
      throw Exception('Failed to stop recording: ${e.message}');
    }
  }

  @override
  Future<void> cancel(String recorderId) async {
    // Cancel is implemented as a call to stop.
    await stop(recorderId);
  }

  @override
  Future<List<InputDevice>> listInputDevices(String recorderId) async {
    // Listing input devices is not implemented in the native plugin.
    return <InputDevice>[];
  }

  @override
  Stream<RecordState> onStateChanged(String recorderId) {
    _stateStreamCtrl ??= StreamController<RecordState>.broadcast();
    return _stateStreamCtrl!.stream;
  }

  void _updateState(RecordState state) {
    if (_state != state) {
      _state = state;
      _stateStreamCtrl?.add(state);
    }
  }
}
